/**
 * @file task.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Task coroutine
 * @version 0.2.3
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
#  include <xsl/coro/core/detach.h>
#  include <xsl/coro/core/executor.h>
#  include <xsl/coro/core/then.h>
#  include <xsl/coro/def.h>
#  include <xsl/log.h>
#  include <xsl/type_traits.h>

#  include <cassert>
#  include <concepts>
#  include <coroutine>
#  include <type_traits>
#  include <utility>

XSL_CORO_NB

class NextBase {
public:
  constexpr NextBase() noexcept(noexcept(std::coroutine_handle<>{nullptr})
                                && noexcept(std::shared_ptr<ExecutorBase>{nullptr}))
      : _next(nullptr), _executor(nullptr) {}

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

  constexpr const std::shared_ptr<ExecutorBase> &executor() const noexcept { return _executor; }

  template <class Awaiter, class... _Args>
    requires(!std::is_reference_v<Awaiter>)
  constexpr std::suspend_never yield_value(Awaiter &&awaiter) {
    coro::detach(std::forward<Awaiter>(awaiter), this->executor());
    return {};
  }

  constexpr void by(this auto &&self,
                    std::convertible_to<std::shared_ptr<ExecutorBase>> auto &&executor) {
    self._executor = std::forward<decltype(executor)>(executor);
  }

  template <class Promise>
  constexpr void next(std::coroutine_handle<Promise> handle) {
    log_trace("Promise next");
    this->_next = handle;
    if constexpr (!std::is_same_v<typename Promise::executor_type, void>) {
      if (!this->executor()) {
        log_trace("set executor");
        this->_executor = handle.promise().executor();
      }
    }
  }

  template <class Promise>
  constexpr void resume(std::coroutine_handle<Promise> handle) {
    if (this->executor()) {
      this->_executor->schedule([handle] mutable {
        log_trace("task resume {}", (uint64_t)handle.address());
        handle();
      });
    } else {
      handle();
    }
  }

protected:
  std::coroutine_handle<> _next;

  std::shared_ptr<ExecutorBase> _executor;
};

template <class ResultType>
class Task;

template <class ResultType>
class TaskPromiseBase : public NextBase, public PromiseBase<ResultType> {
public:
  using coro_type = Task<ResultType>;
  using executor_type = ExecutorBase;
};

template <class ResultType>
class Task {
public:
  using result_type = ResultType;
  using executor_type = ExecutorBase;
  using promise_type = Promise<TaskPromiseBase<ResultType>>;

protected:
  std::coroutine_handle<promise_type> _handle;

  constexpr std::coroutine_handle<promise_type> move_handle() && noexcept {
    return std::exchange(this->_handle, {});
  }

public:
  constexpr Task(std::coroutine_handle<promise_type> handle) noexcept : _handle(handle) {}
  constexpr Task(Task &&task) noexcept : _handle(std::move(task).move_handle()) {}
  constexpr Task &operator=(Task &&task) noexcept {
    _handle = task.move_handle();
    return *this;
  }
  constexpr ~Task() {
    if (_handle) {
      assert(_handle.done());
      _handle.destroy();
    }
  }

  constexpr auto operator co_await(this auto &&self) noexcept(
      std::is_nothrow_move_constructible_v<Task>) {
    log_trace("move handle to Awaiter");
    return std::move(self);
  }

  constexpr auto then(this Task &&self, std::invocable<result_type> auto &&f) {
    return ThenAwaiter<Task>(std::move(self).move_handle()).then(std::forward<decltype(f)>(f));
  }

  template <class Self, class Res = Self::result_type>
    requires(!std::is_reference_v<Self>) && is_same_pack_v<Res, std::expected<void, void>>
  constexpr decltype(auto) and_then(this Self &&self,
                                    std::invocable<typename Res::value_type> auto &&f) {
    return std::move(self).then([f = std::forward<decltype(f)>(f)](auto &&res) {
      return std::forward<decltype(res)>(res).and_then(f);
    });
  }

  template <class Self, class Res = Self::result_type>
    requires(!std::is_reference_v<Self>) && is_same_pack_v<Res, std::expected<void, void>>
  constexpr decltype(auto) map(this Self &&self,
                               std::invocable<typename Res::value_type> auto &&f) {
    return std::move(self).then([f = std::forward<decltype(f)>(f)](auto &&res) {
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
  constexpr result_type block(this auto &&self) {
    log_trace("Task block");
    return coro::block(std::move(self));
  }
  /**
   * @brief Block the task
   *
   * @param self
   * @param executor the executor
   * @return auto&&
   */
  constexpr auto &&by(this auto &&self,
                      std::convertible_to<std::shared_ptr<ExecutorBase>> auto &&executor) {
    self._handle.promise().by(std::forward<decltype(executor)>(executor));
    return std::forward<decltype(self)>(self);
  }
  /// @brief Detach the task
  constexpr void detach(this Task &&self) {
    log_trace("task detach");
    coro::detach(std::move(self));
  }
  /// @brief Detach the task with executor
  constexpr void detach(this Task &&self,
                        std::convertible_to<std::shared_ptr<ExecutorBase>> auto &&executor) {
    coro::detach(std::move(self), std::forward<decltype(executor)>(executor));
  }

  constexpr bool await_ready() const { return false; }

  template <class _Promise>
  constexpr std::coroutine_handle<promise_type> await_suspend(
      std::coroutine_handle<_Promise> handle) {
    log_trace("await_suspend: {} -> {}", (uint64_t)_handle.address(), (uint64_t)handle.address());
    this->_handle.promise().next(handle);
    return this->_handle;
  }

  constexpr result_type await_resume() {
    log_trace("task await_resume for {}", (uint64_t)_handle.address());
    return *_handle.promise();
  }
};
XSL_CORO_NE
#endif  // XSL_CORO_TASK
