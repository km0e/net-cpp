/**
 * @file signal.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Signal for coroutines
 * @version 0.1
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_CORO_SIGNAL
#  define XSL_CORO_SIGNAL
#  include "xsl/coro/def.h"
#  include "xsl/logctl.h"
#  include "xsl/wheel.h"

#  include <cassert>
#  include <coroutine>
#  include <functional>
#  include <mutex>
#  include <optional>
#  include <utility>
XSL_CORO_NB
/**
 * @brief Awaiter for Signal
 *
 * @tparam T type of the signal
 */
template <class T>
class SignalAwaiter {
public:
  using value_type = T;
  SignalAwaiter(std::mutex &mtx, std::optional<value_type> &ready,
                std::optional<std::function<void()>> &cb)
      : _mtx(mtx), _ready(ready), _cb(cb) {}

  bool await_ready() noexcept(noexcept(_mtx.lock())) {
    LOG6("semaphore await_ready");
    this->_mtx.lock();
    return this->_ready.has_value();
  }

  template <class Promise>
  void await_suspend(std::coroutine_handle<Promise> handle) {
    LOG6("semaphore await_suspend for {}", (uint64_t)handle.address());
    this->_cb = [handle] mutable { handle.promise().resume(handle); };
    this->_mtx.unlock();
  }

  [[nodiscard("must use the result of await_resume to confirm the semaphore is ready")]] value_type
  await_resume() {
    // LOG5("semaphore await_resume");
    Defer defer([this] { this->_mtx.unlock(); });  ///< must unlock after return
    return *std::exchange(this->_ready, std::nullopt);
  }

private:
  std::mutex &_mtx;
  std::optional<value_type> &_ready;
  std::optional<std::function<void()>> &_cb;
};

template <class T>
class Signal {
public:
  using value_type = T;
  using awaiter_type = SignalAwaiter<value_type>;

private:
  std::mutex _mtx;
  std::optional<value_type> _ready;
  std::optional<std::function<void()>> _cb;

public:
  using executor_type = void;
  Signal(std::optional<value_type> ready = std::nullopt) : _mtx(), _ready(ready), _cb() {}
  Signal(const Signal &) = delete;
  Signal(Signal &&) = delete;
  Signal &operator=(const Signal &) = delete;
  Signal &operator=(Signal &&) = delete;
  ~Signal() {}

  awaiter_type operator co_await() { return {_mtx, _ready, _cb}; }

  /**
   * @brief Release the signal
   *
   * @tparam _Tp type of the signal
   * @param ready the signal
   */
  template <class _Tp>
  void release(_Tp &&ready) {
    LOG6("semaphore release {}", ready);
    this->_mtx.lock();
    this->_ready = std::forward<_Tp>(ready);
    if (this->_cb) {
      auto cb = std::exchange(
          this->_cb,
          std::nullopt);  // must exchange before call, because the cb may call co_await again
      (*cb)();
    } else {
      this->_mtx.unlock();
    }
  }
};

using BinarySignal = Signal<bool>;  ///< Signal for bool

template <class T>
class UnsafeSignalAwaiter {
public:
  using value_type = T;

  UnsafeSignalAwaiter(std::optional<value_type> &ready, std::function<void()> &cb)
      : _ready(ready), _cb(cb) {}

  bool await_ready() noexcept {
    LOG6("semaphore await_ready");
    return false;
  }

  template <class Promise>
  void await_suspend(std::coroutine_handle<Promise> handle) {
    LOG6("semaphore await_suspend for {}", (uint64_t)handle.address());
    this->_cb = [handle] mutable { handle.promise().resume(handle); };
  }

  [[nodiscard("must use the result of await_resume to confirm the semaphore is ready")]] value_type
  await_resume() {
    LOG6("semaphore await_resume");
    return *std::exchange(this->_ready, std::nullopt);
  }

private:
  std::optional<value_type> &_ready;
  std::function<void()> &_cb;
};

template <class T>
class UnsafeSignal {
public:
  using value_type = T;
  using awaiter_type = UnsafeSignalAwaiter<value_type>;

private:
  std::optional<value_type> _ready;
  std::function<void()> _cb;

public:
  using executor_type = void;
  UnsafeSignal() : _ready(), _cb() {}
  UnsafeSignal(const UnsafeSignal &) = delete;
  UnsafeSignal(UnsafeSignal &&) = delete;
  UnsafeSignal &operator=(const UnsafeSignal &) = delete;
  UnsafeSignal &operator=(UnsafeSignal &&) = delete;
  ~UnsafeSignal() {}

  awaiter_type operator co_await() { return UnsafeSignalAwaiter(_ready, _cb); }

  /**
   * @brief Release the signal
   *
   * @tparam _Tp type of the signal
   * @param ready the signal
   */
  template <class _Tp>
  void release(_Tp &&ready) {
    LOG6("semaphore release {}", ready);
    this->_ready = std::forward<_Tp>(ready);
    auto cb = std::exchange(
        this->_cb, {});  ///< must exchange before call, because the cb may call co_await again
    cb();
  }
};

using UnsafeBinarySignal = UnsafeSignal<bool>;  ///< Signal for bool

XSL_CORO_NE
#endif
