#pragma once
#ifndef XSL_CORO_SEMAPHORE
#  define XSL_CORO_SEMAPHORE
#  include "xsl/coro/def.h"
#  include "xsl/logctl.h"
#  include "xsl/sync/spsc.h"

#  include <functional>
#  include <mutex>
#  include <optional>
#  include <semaphore>
#  include <utility>
XSL_CORO_NB

template <std::ptrdiff_t LeastMaxValue = std::numeric_limits<std::ptrdiff_t>::max()>
class CountingSemaphoreAwaiter {
public:
  CountingSemaphoreAwaiter(std::counting_semaphore<LeastMaxValue> &sem,
                           sync::SPSC<std::function<void()>> &queue)
      : _sem(sem), _queue(queue) {}

  bool await_ready() noexcept(noexcept(_sem.try_acquire())) { return _sem.try_acquire(); }

  template <class Promise>
  void await_suspend(std::coroutine_handle<Promise> handle) {
    LOG5("semaphore await_suspend for {}", (uint64_t)handle.address());
    _queue.push([handle]() mutable {
      LOG5("semaphore resume for {}", (uint64_t)handle.address());
      handle.promise().resume(handle);
    });
  }

  void await_resume() {}

private:
  std::counting_semaphore<LeastMaxValue> &_sem;
  sync::SPSC<std::function<void()>> &_queue;
};

template <std::ptrdiff_t LeastMaxValue = std::numeric_limits<std::ptrdiff_t>::max()>
class CountingSemaphore {
private:
  std::counting_semaphore<LeastMaxValue> _sem;
  sync::SPSC<std::function<void()>> _queue;

public:
  CountingSemaphore(std::ptrdiff_t initial) : _sem(initial), _queue() {}
  CountingSemaphore() : CountingSemaphore(0) {}
  CountingSemaphore(const CountingSemaphore &) = delete;
  CountingSemaphore(CountingSemaphore &&) = delete;
  CountingSemaphore &operator=(const CountingSemaphore &) = delete;
  CountingSemaphore &operator=(CountingSemaphore &&) = delete;
  ~CountingSemaphore() {}

  CountingSemaphoreAwaiter<LeastMaxValue> operator co_await() {
    return CountingSemaphoreAwaiter<LeastMaxValue>(_sem, _queue);
  }

  void release(std::ptrdiff_t update = 1) {
    auto wait_size = this->_queue.size();
    if (update > wait_size) {
      update -= wait_size;
      this->_sem.release(update);
      for (std::ptrdiff_t i = 0; i < wait_size; i++) {
        this->_queue.pop().value()();
      }
    } else {
      for (std::ptrdiff_t i = 0; i < update; i++) {
        this->_queue.pop().value()();
      }
    }
  }
};

template <>
class CountingSemaphoreAwaiter<1> {
public:
  CountingSemaphoreAwaiter(std::mutex &mtx, std::optional<bool> &ready,
                           std::optional<std::function<void()>> &cb)
      : _mtx(mtx), _ready(ready), _cb(cb) {}

  bool await_ready() noexcept(noexcept(_mtx.lock())) {
    LOG5("semaphore await_ready");
    this->_mtx.lock();
    return this->_ready.has_value();
  }

  template <class Promise>
  void await_suspend(std::coroutine_handle<Promise> handle) {
    LOG5("semaphore await_suspend for {}", (uint64_t)handle.address());
    this->_cb = [handle]() mutable { handle.promise().resume(handle); };
    this->_mtx.unlock();
  }

  [[nodiscard("must use the result of await_resume to confirm the semaphore is ready")]] bool
  await_resume() {
    LOG5("semaphore await_resume");
    this->_mtx.unlock();
    return *std::exchange(this->_ready, std::nullopt);
  }

private:
  std::mutex &_mtx;
  std::optional<bool> &_ready;
  std::optional<std::function<void()>> &_cb;
};

template <>
class CountingSemaphore<1> {
private:
  std::mutex _mtx;
  std::optional<bool> _ready;
  std::optional<std::function<void()>> _cb;

public:
  CountingSemaphore(std::optional<bool> ready = std::nullopt) : _mtx(), _ready(ready), _cb() {}
  CountingSemaphore(const CountingSemaphore &) = delete;
  CountingSemaphore(CountingSemaphore &&) = delete;
  CountingSemaphore &operator=(const CountingSemaphore &) = delete;
  CountingSemaphore &operator=(CountingSemaphore &&) = delete;
  ~CountingSemaphore() {}

  CountingSemaphoreAwaiter<1> operator co_await() {
    return CountingSemaphoreAwaiter<1>(_mtx, _ready, _cb);
  }

  void release(bool ready = true) {
    LOG5("semaphore release");
    this->_mtx.lock();
    this->_ready = ready;
    if (this->_cb) {
      auto cb = std::exchange(this->_cb, std::nullopt);
      (*cb)();
    } else {
      this->_mtx.unlock();
    }
  }
};
XSL_CORO_NE
#endif
