/**
 * @file mpsc.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief MPSC signal for coroutines
 * @version 0.2
 * @date 2024-09-14
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_CORO_SIGNAL_MPSC
#  define XSL_CORO_SIGNAL_MPSC
#  include <xsl/coro/def.h>
#  include <xsl/coro/log.h>
#  include <xsl/coro/signal/def.h>
#  include <xsl/coro/signal/unsafe.h>
#  include <xsl/wheel.h>
XSL_CORO_NB

struct SignalStorage {
  using max_signals = UnsafeSignalStorage::max_signals;
  SignalStorage() : mtx(), _unsafe() {}
  std::mutex mtx;
  UnsafeSignalStorage _unsafe;
};
template <>
struct SignalAwaiterTraits<SignalStorage> {
  using storage_type = SignalStorage;
  using unsafe_awaiter_type = SignalAwaiter<UnsafeSignalStorage>;
  /**
   * @brief Check if the signal is ready
   *
   * @return true if the signal is ready
   * @return false if the signal is not ready
   */
  constexpr bool await_ready(this auto &self) {
    self.storage.mtx.lock();
    return self.awaiter().await_ready();
  }
  /**
   * @brief Suspend the signal
   *
   * @tparam Promise the promise type
   * @param handle the coroutine handle
   */
  template <class Promise>
  constexpr void await_suspend(this auto &self, std::coroutine_handle<Promise> handle) {
    self.awaiter().await_suspend(handle);
    self.storage.mtx.unlock();
    co_trace("Signal suspended");
  }
  /**
   * @brief Resume the signal
   *
   * @return true if the signal is still alive
   * @return false if the signal is not alive
   */
  [[nodiscard("must use the result of await_resume to confirm the signal is still alive")]]
  constexpr bool await_resume(this auto &&self) {
    Defer defer{[&self] { self.storage.mtx.unlock(); }};
    return self.awaiter().await_resume();
  }

private:
  unsafe_awaiter_type awaiter(this auto &&self) {
    return unsafe_awaiter_type(self.storage._unsafe);
  }
};

template <std::ptrdiff_t MaxSignals>
struct SignalTraits<SignalStorage, MaxSignals> {
  using storage_type = SignalStorage;
  using max_signals = std::integral_constant<std::ptrdiff_t, MaxSignals>;

private:
  struct SignalRef : public SignalTraits<UnsafeSignalStorage, MaxSignals> {
    friend struct SignalTraits<UnsafeSignalStorage, MaxSignals>;
    UnsafeSignalStorage &storage;
    SignalRef(UnsafeSignalStorage &storage) : storage(storage) {}
  };

public:
  /**
   * @brief Release the signal
   *
   * @param storage the signal storage
   */
  constexpr bool release(this auto &self) {
    self.storage.mtx.lock();
    bool result = self.unsafe_ref().release();
    if (!result) {
      self.storage.mtx.unlock();
    }
    return result;
  }
  /**
   * @brief Stop the signal
   *
   * @return true if the signal is stopped successfully, false if the signal is not alive
   */
  constexpr bool stop(this auto &self) {
    self.storage.mtx.lock();
    bool result = self.unsafe_ref().stop();
    if (!result) {
      self.storage.mtx.unlock();
    }
    return result;
  }

  /**
   * @brief Force stop the signal
   *
   * @return std::optional<std::ptrdiff_t> the number of signals that are stopped, or std::nullopt
   * if the signal is not alive
   */
  constexpr std::optional<std::ptrdiff_t> force_stop(this auto &self) {
    self.storage.mtx.lock();
    std::optional<std::ptrdiff_t> result = self.unsafe_ref().force_stop();
    if (result.has_value()) {
      self.storage.mtx.unlock();
      return result;
    }
    return std::nullopt;
  }

private:
  SignalRef unsafe_ref(this auto &&self) { return self.storage._unsafe; }
};
XSL_CORO_NE
#endif
