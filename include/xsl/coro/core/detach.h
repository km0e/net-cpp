/**
 * @file detach.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Detach a coroutine
 * @version 0.1.3
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_CORO_DETACH
#  define XSL_CORO_DETACH
#  include <xsl/coro/core/base.h>
#  include <xsl/coro/core/context.h>
#  include <xsl/coro/core/executor.h>
#  include <xsl/coro/def.h>
#  include <xsl/type_traits.h>
#  include <xsl/wheel.h>

#  include <cassert>
#  include <coroutine>
#  include <type_traits>
#  include <utility>
XSL_CORO_NB
template <class ResultType>
class Detach;

template <class ResultType>
class DetachPromiseBase : public PromiseBase<ResultType> {
public:
  using has_context = std::bool_constant<true>;
  using coro_type = Detach<ResultType>;
  DetachPromiseBase() noexcept : _ctx() {}

  constexpr std::suspend_always initial_suspend() noexcept { return {}; }
  constexpr std::suspend_never final_suspend() noexcept {
    // @note must NOT destroy the frame here: with suspend_never the runtime
    //       destroys the coroutine state after final_suspend completes, so a
    //       manual destroy would double-free the frame
    return {};
  }

  /// @brief a detached task's exception is unobservable — at least log it
  ///        instead of dropping it silently
  void unhandled_exception() noexcept {
    auto eptr = std::current_exception();
    try {
      if (eptr) std::rethrow_exception(eptr);
    } catch (const std::exception &e) {
      co_error("detached task died with exception: {}", e.what());
    } catch (...) {
      co_error("detached task died with unknown exception");
    }
    this->_result = std::move(eptr);
  }

  constexpr void by(this auto&& self, auto&& ctx, std::coroutine_handle<> self_handle) noexcept(
      std::is_nothrow_assignable_v<Rc<CoroContext>, decltype(ctx)>) {
    self._ctx = std::forward<decltype(ctx)>(ctx);
    self._self = self_handle;
  }

  constexpr auto ctx(this auto&& self) noexcept -> like_t<decltype(self), Rc<CoroContext>> {
    return self._ctx;
  }

protected:
  Rc<CoroContext> _ctx;
  std::coroutine_handle<> _self = nullptr;
};
template <class ResultType>
class Detach {
public:
  using promise_type = Promise<DetachPromiseBase<ResultType>>;

  constexpr explicit Detach(std::coroutine_handle<promise_type> handle) noexcept
      : _handle(handle) {}

  constexpr Detach(Detach&& ano) noexcept : _handle(std::exchange(ano._handle, {})) {}

  constexpr ~Detach() {
    assert(!_handle && "Detach dropped without being invoked");
    if (_handle) {
      // never invoked: reclaim the never-started frame (still suspended at
      // initial_suspend) instead of leaking it
      _handle.destroy();
    }
  }

  constexpr void operator()(this auto self, Rc<CoroContext>&& ctx) noexcept {
    co_trace("detach");
    // A detached task starts a NEW coroutine chain: the chain root must own its
    // context exclusively (Rc ref_count is non-atomic by design, see
    // docs/architecture.md §3.5). Checked here in debug builds; at this point
    // the task has not been dispatched yet, so reading the count is race-free.
    assert(ctx.use_count() == 1
           && "detach: ctx must be uniquely owned — pass a CoroContext or an "
              "exclusively-owned Rc (docs/architecture.md §3.5)");
    self._handle.promise().by(std::forward<decltype(ctx)>(ctx), self._handle);
    self._handle.promise().ctx()->dispatch([handle = self._handle]() mutable {
      co_trace("detach resume {}", (uint64_t)handle.address());
      handle.resume();
    });
    // ownership transferred to the frame itself (final_suspend = suspend_never
    // destroys it); release it so ~Detach does not reclaim a RUNNING frame
    self._handle = nullptr;
  }

private:
  std::coroutine_handle<promise_type> _handle;
};

/**
 * @brief Detach an awaiter to run independently (fire-and-forget)
 * @param awaiter the awaiter to detach (moved into the detached frame)
 * @param ctx context for the new chain root — executor, reserved object and
 *        cancellation state all reach the chain through it
 * @pre `ctx` must be uniquely owned (`ctx.unique()`): the Rc ref_count is
 *      non-atomic by design, sharing the inner context with anything outside
 *      the new chain is a data race (docs/architecture.md §3.5). Pass a
 *      CoroContext (a fresh Rc is built) or an exclusively-owned Rc.
 *      Checked by assertion in debug builds.
 */
template <class Awaiter>
  requires Awaitable<Awaiter, Detach<typename Awaiter::result_type>>
           && (!std::is_reference_v<Awaiter>)
constexpr void detach(Awaiter&& awaiter, Rc<CoroContext>&& ctx) {
  auto d = [](Awaiter awaiter) -> Detach<typename awaiter_traits<Awaiter>::result_type> {
    co_trace("detach");
    co_await std::move(awaiter);
  }(std::forward<Awaiter>(awaiter));
  std::move(d)(std::move(ctx));
}

XSL_CORO_NE
#endif
