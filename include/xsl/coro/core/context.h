/**
 * @file context.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Coroutine context
 * @version 0.1.0
 * @date 2025-09-14
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once

#ifndef XSL_CORO_CONTEXT
#  define XSL_CORO_CONTEXT
#  include <xsl/coro/core/executor.h>
#  include <xsl/coro/def.h>
#  include <xsl/coro/log.h>

#  include <atomic>
#  include <cassert>
#  include <functional>
#  include <memory>

XSL_CORO_NB

enum class CancelState { None, Pending, Yes };
using Continuation = std::function<void()>;

class CoroContext {
  std::shared_ptr<ExecutorBase> _e = std::make_shared<NoopExecutor>();
  std::shared_ptr<void> _reserved;

  CoroContext(const std::shared_ptr<ExecutorBase>& e,
              const std::shared_ptr<void>& reserved = {})
      : _e(e), _reserved(reserved) {}

public:
  CoroContext() = default;
  template <Executor E>
  CoroContext(E&& executor, void* reserved = nullptr, void (*deleter)(void*) = nullptr)
      : _e(std::make_shared<E>(std::forward<E>(executor)))
      , _reserved(reserved, deleter ? deleter : [](void*) {}) {}
  ~CoroContext() = default;

  void dispatch(move_only_function<void()>&& func) {
    co_trace("Context dispatching");
    _e->schedule(std::move(func));
  }
  /// @brief create a child context sharing the executor and the reserved object
  /// @note must return by value; returning a reference would dangle
  CoroContext new_child_context() { return CoroContext(_e, _reserved); }

  constexpr auto get_reserved() { return _reserved.get(); }

  constexpr auto& cc() noexcept { return _cc; }
  constexpr auto& cs() noexcept { return _cs; }

  constexpr Continuation* set_cc(Continuation* cc) noexcept {
    return _cc->exchange(cc, std::memory_order_acq_rel);
  }

  /// Cancel all operations on this context — sets CancelState and wakes suspended coroutines
  void cancel() {
    _cs->store(CancelState::Yes, std::memory_order_release);
    auto c = _cc->exchange(nullptr, std::memory_order_acq_rel);
    if (c) {
      (*c)();
      delete c;
    }
  }

private:
  std::shared_ptr<std::atomic<Continuation*>> _cc
      = std::make_shared<std::atomic<Continuation*>>(nullptr);
  std::shared_ptr<std::atomic<CancelState>> _cs
      = std::make_shared<std::atomic<CancelState>>(CancelState::None);

};

template <typename Signal>
struct Cancellable { Signal& _sig; };

template <typename Signal>
Cancellable<Signal> cancellable(Signal& sig) { return {sig}; }

template <class T>
struct Reserved {
  mutable void* _ptr = nullptr;
  constexpr bool await_ready() const noexcept { return true; }
  template <class Promise>
  constexpr void await_suspend(std::coroutine_handle<Promise> handle) const noexcept {
    _ptr = handle.promise().ctx()->get_reserved();
    assert(_ptr != nullptr);
  }
  /// @brief resume with the reserved object of the current coroutine context
  /// @note await_transform wraps this awaiter with the promise's CoroContext,
  ///       so the context-aware overload is preferred; the no-argument overload
  ///       only works when await_suspend actually ran
  constexpr T& await_resume(CoroContext& ctx) const noexcept {
    return *static_cast<T*>(ctx.get_reserved());
  }
  constexpr T& await_resume() const noexcept { return *static_cast<T*>(_ptr); }
};

XSL_CORO_NE
#endif  // XSL_CORO_CONTEXT
