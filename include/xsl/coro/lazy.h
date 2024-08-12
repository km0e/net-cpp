#pragma once
#ifndef XSL_CORO_LAZY
#  define XSL_CORO_LAZY
#  include "xsl/coro/base.h"
#  include "xsl/coro/def.h"
#  include "xsl/coro/detach.h"
#  include "xsl/coro/executor.h"
#  include "xsl/coro/next.h"
#  include "xsl/logctl.h"

#  include <cassert>
#  include <coroutine>
#  include <expected>
XSL_CORO_NB
template <class Promise>
class LazyAwaiter : public NextAwaiter<Promise> {
private:
  using Base = NextAwaiter<Promise>;

protected:
  using promise_type = Promise;

public:
  using typename Base::result_type;

  using Base::Base;
  bool await_ready() {
    _handle.promise().resume(_handle);
    return _handle.done();
  }

protected:
  using Base::_handle;
};

template <class ResultType, class Executor = ExecutorBase>
class Lazy;

template <class ResultType, class Executor>
class LazyPromiseBase : public NextPromiseBase<ResultType, Executor> {
private:
  using Base = NextPromiseBase<ResultType, Executor>;

public:
  using typename Base::result_type;
  using coro_type = Lazy<ResultType, Executor>;
  std::suspend_always initial_suspend() noexcept {
    LOG6("initial_suspend");
    return {};
  }
};

template <class ResultType, class Executor>
class Lazy : public Next<ResultType, Executor> {
private:
  using LazyBase = Next<ResultType, Executor>;

protected:
  friend typename LazyBase::Friend;

public:
  using promise_type = Promise<LazyPromiseBase<ResultType, Executor>>;

  using executor_type = promise_type::executor_type;
  using result_type = promise_type::result_type;
  using awaiter_type = LazyAwaiter<promise_type>;

  static_assert(Awaitable<awaiter_type>, "LazyAwaiter is not Awaitable");

  explicit Lazy(std::coroutine_handle<promise_type> handle) noexcept : _handle(handle) {}

  Lazy(Lazy &&task) noexcept : _handle(std::exchange(task._handle, {})) {}

  ~Lazy() { assert(!_handle); }

  void detach(this auto &&self) {
    LOG6("task detach");
    _coro::detach(std::move(self));
  }

  /**
   * @brief detach the task with the executor
   *
   * @tparam E the executor type
   * @param self the task
   * @param executor the executor
   * @return void
   */
  template <class E>
    requires std::constructible_from<std::shared_ptr<Executor>, E>
  void detach(this auto &&self, E &&executor) {
    self.by(std::forward<E>(executor)).detach();
  }

protected:
  std::coroutine_handle<promise_type> _handle;
};

static_assert(Awaitable<Lazy<int>>, "Lazy<int> is not Awaitable");

XSL_CORO_NE
#endif
