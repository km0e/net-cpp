/**
 * @file core.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Unified coroutine signal — one state machine, two contention tiers
 * @version 0.5.0
 * @date 2025-09-14
 *
 * @copyright Copyright (c) 2025
 *
 * Both tiers implement the SAME state machine and the same semantics:
 *
 *   IDLE     -> no pending signal, no waiter
 *   SIGNALED -> a release was observed before any consumer suspended
 *   STOPPED  -> sticky terminal state; await resumes false, release is ignored
 *   WAITING  -> a consumer is suspended (continuation registered)
 *
 * The tiers differ only in synchronization:
 *   - UnsafeSignal: single-threaded, zero synchronization
 *   - MPSCSignal:   lock-free atomics; release()/stop() are safe to call from
 *                   ANY number of producer threads (a single producer is just
 *                   the degenerate case, so this also covers SPSC). Concurrent
 *                   releases coalesce; a spurious wakeup is filtered out by the
 *                   consumer re-checking the state.
 *
 * Cancellation from another thread (e.g. Cancellable) is only valid for the
 * atomic tier — it makes the cancelling thread an additional producer.
 *
 * The continuation is stored as two POD fields ({resume thunk, handle}) INSIDE
 * the signal — zero allocation per suspend. This is sound because the tier
 * contract is single-consumer: at most one await is outstanding per signal.
 * Producers/stop copy the fields to their stack BEFORE invoking, so an inline
 * resume (NoopExecutor) that immediately re-awaits and overwrites them is safe.
 */
#pragma once
#ifndef XSL_CORO_SIGNAL_CORE
#  define XSL_CORO_SIGNAL_CORE
#  include <xsl/coro/core/context.h>
#  include <xsl/coro/def.h>

#  include <atomic>
#  include <coroutine>
#  include <cstdint>
#  include <utility>

XSL_CORO_NB

/**
 * @brief Single-threaded signal — zero synchronization
 *
 * await_resume returns true on signal, false after stop().
 * stop() is sticky: subsequent awaits observe the stop immediately.
 */
class UnsafeSignal {
  enum : std::uintptr_t { IDLE, SIGNALED, STOPPED, WAITING };
  std::uintptr_t _state{IDLE};
  // per-suspend continuation (single consumer: at most one outstanding await)
  void (*_resume)(void*) = nullptr;
  void* _handle = nullptr;

public:
  constexpr bool await_ready() noexcept {
    if (_state == SIGNALED) {
      _state = IDLE;
      return true;
    }
    return _state == STOPPED;
  }
  template <class Promise>
  bool await_suspend(std::coroutine_handle<Promise> handle) noexcept {
    _resume = [](void* p) {
      auto h = std::coroutine_handle<Promise>::from_address(p);
      h.promise().resume(h);
    };
    _handle = handle.address();
    _state = WAITING;
    return true;  // single thread: nothing can interleave with await_ready
  }
  [[nodiscard("false means the signal was stopped")]]
  constexpr bool await_resume() const noexcept {
    return _state != STOPPED;
  }
  /// @brief release the signal (same thread only)
  void release() noexcept {
    if (_state == IDLE) {
      _state = SIGNALED;
      return;
    }
    if (_state != WAITING) return;  // coalesce; stop is sticky
    _state = IDLE;
    // copy the continuation out BEFORE invoking it: the callback resumes the
    // consumer inline, which may immediately re-await and overwrite the fields
    auto fn = _resume;
    auto* h = _handle;
    fn(h);
  }
  /// @brief sticky stop; wakes the suspended consumer (if any) with false
  void stop() noexcept {
    if (_state == STOPPED) return;
    bool waiting = _state == WAITING;
    auto fn = _resume;
    auto* h = _handle;
    _state = STOPPED;
    if (waiting) fn(h);
  }
  constexpr bool stopped() const noexcept { return _state == STOPPED; }
};

/**
 * @brief Multi-producer (single-consumer) signal — lock-free atomic tier
 *
 * WAITING is a plain sentinel; the continuation lives in the {_resume,_handle}
 * fields (consumer-owned, single outstanding await). Ownership of the WAITING
 * slot is ALWAYS transferred via exchange: whoever observes WAITING owns the
 * right to invoke the continuation — and copies the fields to its stack first,
 * so the inline-resume/re-await overwrite is safe. Retraction in await_suspend
 * via the second exchange never races with that copy-out.
 */
class MPSCSignal {
  static constexpr std::uintptr_t IDLE = 0, SIGNALED = 1, STOPPED = 2, WAITING = 3;
  std::atomic<std::uintptr_t> _state{IDLE};
  // per-suspend continuation (single consumer: at most one outstanding await)
  void (*_resume)(void*) = nullptr;
  void* _handle = nullptr;

public:
  bool await_ready() noexcept {
    if (_state.load(std::memory_order_acquire) == SIGNALED) {
      // consume; failure means a concurrent stop/re-signal — still ready
      std::uintptr_t expected = SIGNALED;
      _state.compare_exchange_strong(expected, IDLE, std::memory_order_acquire);
      return true;
    }
    return _state.load(std::memory_order_acquire) == STOPPED;
  }
  /// @brief register the continuation and suspend
  /// @return true to suspend, false if a signal/stop arrived in the window
  ///         between await_ready and here (consumed inline)
  template <class Promise>
  bool await_suspend(std::coroutine_handle<Promise> handle) noexcept {
    _resume = [](void* p) {
      auto h = std::coroutine_handle<Promise>::from_address(p);
      h.promise().resume(h);
    };
    _handle = handle.address();
    // RULE (cppreference, [expr.await]): once the handle is published here,
    // another thread may resume (and even complete/destroy) the coroutine
    // while await_suspend is still executing — that scenario is NOT UB, but
    // this function must not touch anything owned by the coroutine frame
    // afterwards (the exchange below only touches this signal's own state).
    auto old = _state.exchange(WAITING, std::memory_order_acq_rel);
    if (old == IDLE) return true;  // registered, waiting
    // old was SIGNALED or STOPPED: consume it, restoring STOPPED (sticky),
    // and retract the registration atomically
    auto got = _state.exchange(old == STOPPED ? STOPPED : IDLE, std::memory_order_acq_rel);
    return got != WAITING;  // retracted -> don't suspend; else producer owns it
  }
  [[nodiscard("false means the signal was stopped")]]
  bool await_resume() const noexcept {
    return _state.load(std::memory_order_acquire) != STOPPED;
  }
  /// @brief release the signal — safe from any number of producer threads
  void release() noexcept {
    auto old = _state.exchange(SIGNALED, std::memory_order_acq_rel);
    if (old == IDLE || old == SIGNALED) return;  // no waiter / coalesce
    if (old == STOPPED) {                        // stop is sticky, restore it
      _state.store(STOPPED, std::memory_order_release);
      return;
    }
    // old == WAITING: we own the continuation — copy it out BEFORE invoking
    auto fn = _resume;
    auto* h = _handle;
    _state.store(IDLE, std::memory_order_release);
    fn(h);
  }
  /// @brief sticky stop; wakes the suspended consumer (if any) with false
  void stop() noexcept {
    auto old = _state.exchange(STOPPED, std::memory_order_acq_rel);
    if (old == IDLE || old == SIGNALED || old == STOPPED) return;
    // old == WAITING: copy the continuation out BEFORE invoking
    auto fn = _resume;
    auto* h = _handle;
    fn(h);  // state stays STOPPED
  }
  bool stopped() const noexcept { return _state.load(std::memory_order_acquire) == STOPPED; }
};

XSL_CORO_NE
#endif
