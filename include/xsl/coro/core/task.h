/**
 * @file task.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Task coroutine
 * @version 0.3.0
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_CORO_TASK
#  define XSL_CORO_TASK
#  include <xsl/coro/core/base.h>
#  include <xsl/coro/core/block.h>
#  include <xsl/coro/core/context.h>
#  include <xsl/coro/core/detach.h>
#  include <xsl/coro/core/executor.h>
#  include <xsl/coro/core/then.h>
#  include <xsl/coro/def.h>
#  include <xsl/type_traits.h>
#  include <xsl/wheel.h>

#  include <cassert>
#  include <concepts>
#  include <coroutine>
#  include <type_traits>
#  include <utility>

XSL_CORO_NB

template <class Awaiter>
struct AwaiterWrapper {
  Awaiter awaiter;
  CoroContext& ctx;

  template <class _Awaiter>
  explicit AwaiterWrapper(_Awaiter&& awaiter, CoroContext& ctx) noexcept
      : awaiter(std::forward<_Awaiter>(awaiter)), ctx(ctx) {}

  constexpr bool await_ready() noexcept(noexcept(std::declval<Awaiter>().await_ready())) {
    return awaiter.await_ready();
  }
  template <class Promise>
  constexpr auto await_suspend(std::coroutine_handle<Promise> handle) noexcept(
      noexcept(std::declval<Awaiter>().await_suspend(handle))) {
    return awaiter.await_suspend(handle);
  }
  constexpr decltype(auto) await_resume() {
    if constexpr (requires { awaiter.await_resume(ctx); }) {
      return awaiter.await_resume(ctx);
    } else {
      return awaiter.await_resume();
    }
  }
};

template <class Awaiter>
  requires(std::is_lvalue_reference_v<Awaiter>)
AwaiterWrapper(Awaiter&&, CoroContext&) -> AwaiterWrapper<Awaiter>;

template <class Awaiter>
  requires(!std::is_reference_v<Awaiter>)
AwaiterWrapper(Awaiter&&, CoroContext&) -> AwaiterWrapper<Awaiter&&>;

class NextBase {
public:
  constexpr NextBase() noexcept : _ctx(), _next(nullptr) {}

  constexpr std::suspend_always initial_suspend() const noexcept { return {}; }

  struct final_awaiter {
    constexpr bool await_ready() const noexcept { return false; }
    constexpr std::coroutine_handle<> await_suspend(std::coroutine_handle<>) const noexcept {
      return _next;
    }
    constexpr void await_resume() const noexcept {}
    std::coroutine_handle<> _next;
  };

  constexpr final_awaiter final_suspend() const noexcept { return {this->_next}; }

  template <class Awaiter, class... _Args>
    requires(!std::is_reference_v<Awaiter>)
  constexpr std::suspend_never yield_value(Awaiter&& awaiter) {
    coro::detach(std::forward<Awaiter>(awaiter), Rc(this->_ctx->new_child_context()));
    return {};
  }

  template <class Awaiter>
  constexpr decltype(auto) await_transform(Awaiter&& awaiter) {
    return AwaiterWrapper(std::forward<Awaiter>(awaiter), *this->_ctx);
  }

  constexpr auto ctx(this auto&& self) noexcept -> like_t<decltype(self), Rc<CoroContext>> {
    return self._ctx;
  }

  /// @pre if an Rc<CoroContext> is passed it must be uniquely owned — the
  ///      ref_count is non-atomic by design (docs/architecture.md §3.5);
  ///      checked by assertion in debug builds
  template <class... Args>
    requires std::constructible_from<Rc<CoroContext>, Args&&...>
  constexpr void by(this auto&& self, Args&&... args) {
    co_trace("Promise by");
    self._ctx = Rc<CoroContext>(std::forward<Args>(args)...);
    assert(self._ctx.unique()
           && "by: ctx must be uniquely owned (docs/architecture.md §3.5)");
  }
  /// @brief check whether a context has been attached to this promise
  constexpr bool has_ctx(this auto&& self) noexcept { return static_cast<bool>(self._ctx); }

  template <class Promise>
  constexpr void next(this auto&& self, std::coroutine_handle<Promise> handle) {
    co_trace("Promise next");
    self._next = handle;
    if constexpr (requires { handle.promise().ctx(); }) {
      co_trace("set executor");
      self._ctx = handle.promise().ctx();
    }
  }

  template <class Promise>
  constexpr void resume(std::coroutine_handle<Promise> handle) {
    this->_ctx->dispatch([handle] mutable {
      co_trace("task resume {}", (uint64_t)handle.address());
      handle();
    });
  }

protected:
  Rc<CoroContext> _ctx;
  std::coroutine_handle<> _next;
};

template <class ResultType>
class Task;

template <class ResultType>
class TaskPromiseBase : public NextBase, public PromiseBase<ResultType> {
public:
  using coro_type = Task<ResultType>;
};

template <class ResultType>
class Task {
public:
  using result_type = ResultType;
  using promise_type = Promise<TaskPromiseBase<ResultType>>;

protected:
  std::coroutine_handle<promise_type> _handle;

  constexpr std::coroutine_handle<promise_type> move_handle() && noexcept {
    return std::exchange(this->_handle, {});
  }

public:
  constexpr Task(std::coroutine_handle<promise_type> handle) noexcept : _handle(handle) {}
  Task(const Task&) = delete;
  Task& operator=(const Task&) = delete;
  constexpr Task(Task&& task) noexcept : _handle(std::move(task).move_handle()) {}
  constexpr Task& operator=(Task&& task) noexcept {
    _handle = task.move_handle();
    return *this;
  }
  constexpr ~Task() {
    if (_handle) {
      assert(_handle.done() && "Task destroyed before co_return/co_yield");
      _handle.destroy();
    }
  }

  // operator co_await removed: Task itself is the awaiter.
  // This avoids a GCC coroutine codegen bug that double-destroys the
  // co_await temporary during coroutine frame cleanup.

  constexpr auto then(this Task&& self, std::invocable<result_type> auto&& f) {
    return ThenAwaiter<Task>(std::move(self).move_handle()).then(std::forward<decltype(f)>(f));
  }

  template <class Self, class Res = Self::result_type>
    requires(!std::is_reference_v<Self>) && is_same_pack_v<Res, std::expected<void, void>>
  constexpr decltype(auto) and_then(this Self&& self,
                                    std::invocable<typename Res::value_type> auto&& f) {
    return std::move(self).then([f = std::forward<decltype(f)>(f)](auto&& res) {
      return std::forward<decltype(res)>(res).and_then(f);
    });
  }

  template <class Self, class Res = Self::result_type>
    requires(!std::is_reference_v<Self>) && is_same_pack_v<Res, std::expected<void, void>>
            && std::is_void_v<typename Res::value_type>
  constexpr decltype(auto) map(this Self&& self, std::invocable<> auto&& f) {
    return std::move(self).then([f = std::forward<decltype(f)>(f)](auto&& res) {
      return std::forward<decltype(res)>(res).transform(f);
    });
  }

  template <class Self, class Res = Self::result_type>
    requires(!std::is_reference_v<Self>) && is_same_pack_v<Res, std::expected<void, void>>
  constexpr decltype(auto) map(this Self&& self,
                               std::invocable<typename Res::value_type> auto&& f) {
    return std::move(self).then([f = std::forward<decltype(f)>(f)](auto&& res) {
      return std::forward<decltype(res)>(res).transform(f);
    });
  }

  /**
   * @brief Block the task
   *
   * @tparam Self
   * @param self
   * @return result_type
   */
  constexpr decltype(auto) block(this auto&& self)
    requires(std::is_rvalue_reference_v<decltype(self)>)
  {
    co_trace("Task block");
    // only fall back to an empty context, never clobber one set via .by(ctx)
    if (!self._handle.promise().has_ctx()) {
      self._handle.promise().by(CoroContext{});
    }
    return coro::block(std::move(self));
  }
  /**
   * @brief Attach a context to the task
   *
   * @param self
   * @param args forwarded to construct the Rc<CoroContext>
   * @return auto&&
   * @pre if an Rc<CoroContext> is passed it must be uniquely owned
   *      (docs/architecture.md §3.5); checked by assertion in debug builds
   */
  template <class... Args>
    requires std::constructible_from<Rc<CoroContext>, Args&&...>
  constexpr auto&& by(this auto&& self, Args&&... args) {
    self._handle.promise().by(Rc<CoroContext>(std::forward<Args>(args)...));
    return std::forward<decltype(self)>(self);
  }
  /// @brief Detach the task with a context (executor / reserved / cancellation)
  /// @note at least one argument is required: without a CoroContext the
  ///       detached task would dispatch on a null context and crash
  /// @pre if an Rc<CoroContext> is passed it must be uniquely owned — a
  ///      detached task starts a new chain and the ref_count is non-atomic
  ///      by design (docs/architecture.md §3.5); checked by assertion in
  ///      debug builds
  template <class... Args>
    requires std::constructible_from<Rc<CoroContext>, Args&&...>
  constexpr void detach(this auto&& self, Args&&... args)
    requires(std::is_rvalue_reference_v<decltype(self)>)
  {
    static_assert(sizeof...(Args) != 0,
                  "Task::detach requires a CoroContext (e.g. detach(ctx)); "
                  "detaching without a context would crash at runtime");
    coro::detach(std::move(self), Rc<CoroContext>(std::forward<Args>(args)...));
    co_debug("Task detached");
  }
  constexpr std::shared_ptr<std::atomic<Continuation*>>& cc() noexcept {
    return this->_handle.promise().ctx()->cc();
  }

  constexpr bool await_ready() const { return false; }

  template <class _Promise>
  constexpr std::coroutine_handle<promise_type> await_suspend(
      std::coroutine_handle<_Promise> handle) {
    co_trace("await_suspend: {} -> {}", (uint64_t)_handle.address(), (uint64_t)handle.address());
    this->_handle.promise().next(handle);
    return this->_handle;
  }

  constexpr result_type await_resume() {
    co_trace("task await_resume for {}", (uint64_t)_handle.address());
    return *_handle.promise();
  }
};
XSL_CORO_NE
#endif  // XSL_CORO_TASK
