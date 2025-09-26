/**
 * @file spsc4.h
 * @brief Minimal SPSC boolean signal — single atomic<uintptr_t> CAS state machine
 * @version 0.4.0
 *
 * States: 0=idle, 1=signaled, ptr=waiting(consumer suspended).
 * All transitions via atomic exchange — no double-check needed.
 */

#pragma once
#ifndef XSL_CORO_SIGNAL_SPSC4
#  define XSL_CORO_SIGNAL_SPSC4

#  include <xsl/compose.h>
#  include <xsl/coro/def.h>
#  include <xsl/coro/signal/def.h>
#  include <xsl/wheel.h>

#  include <atomic>
#  include <cstdint>
#  include <memory>

XSL_CORO_NB

struct SPSCSignalStorage4 {
  std::atomic<uintptr_t> state = 0;  // 0=idle, 1=signaled, ptr=waiting

  constexpr bool await_ready(this auto& self) noexcept {
    uintptr_t expected = 1;
    return self.state.compare_exchange_strong(expected, 0, std::memory_order_acquire);
  }

  template <class Promise>
  constexpr auto await_suspend(this auto& self, std::coroutine_handle<Promise> handle) {
    auto f = new coro::Continuation([handle]() { handle.promise().resume(handle); });
    auto old = self.state.exchange(reinterpret_cast<uintptr_t>(f), std::memory_order_acq_rel);
    if (old == 1) {
      self.state.store(0, std::memory_order_release);
      delete f;
      return false;
    }
    return true;
  }

  constexpr bool await_resume(this auto&) { return true; }

  constexpr void release(this auto& self) {
    auto old = self.state.exchange(1, std::memory_order_acq_rel);
    if (old != 0 && old != 1) {
      auto f = reinterpret_cast<coro::Continuation*>(old);
      auto _ = std::unique_ptr<coro::Continuation>(f);
      self.state.store(0, std::memory_order_release);
      (*f)();
    }
  }
};

using SPSCSignal4 = SPSCSignalStorage4;

XSL_CORO_NE
XSL_CORO_NB
template <>
struct Cancellable<SPSCSignal4> {
  SPSCSignal4& _sig;
  std::shared_ptr<std::atomic<coro::CancelState>> _cs
      = std::make_shared<std::atomic<coro::CancelState>>(coro::CancelState::None);

  constexpr bool await_ready(this auto& self) noexcept { return self._sig.await_ready(); }

  template <class Promise>
  constexpr auto await_suspend(this auto& self, std::coroutine_handle<Promise> handle) {
    Rc<coro::CoroContext>& ctx = handle.promise().ctx();

    auto cs = self._cs;
    auto& sig = self._sig;
    auto wakeup = new coro::Continuation([cs, &sig]() {
      cs->store(coro::CancelState::Yes, std::memory_order_release);
      sig.release();
    });
    ctx->set_cc(wakeup);

    if (ctx->cs()->load(std::memory_order_acquire) == coro::CancelState::Yes) {
      self._cs->store(coro::CancelState::Yes, std::memory_order_release);
      delete ctx->set_cc(nullptr);
      return false;
    }

    return self._sig.await_suspend(handle);
  }

  constexpr bool await_resume(this auto& self) {
    if (self._cs->load(std::memory_order_acquire) == coro::CancelState::Yes) return false;
    return self._sig.await_resume();
  }
};
XSL_CORO_NE
#endif
