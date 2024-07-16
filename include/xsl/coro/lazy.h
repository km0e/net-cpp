#pragma once
#ifndef XSL_CORO_LAZY
#  define XSL_CORO_LAZY
#  include "xsl/coro/def.h"
#  include "xsl/coro/executor.h"
#  include "xsl/coro/next.h"
#  include "xsl/logctl.h"

#  include <cassert>
#  include <coroutine>
#  include <expected>
XSL_CORO_NB
template <class Promise>
class LazyAwaiter : public Awaiter<Promise> {
  using Base = Awaiter<Promise>;

public:
  using promise_type = typename Base::promise_type;
  using result_type = typename Base::result_type;
  using Base::Base;
  bool await_ready() {
    _handle.promise().resume(_handle);
    return _handle.done();
  }

protected:
  using Base::_handle;
};

template <typename ResultType, typename Executor = NoopExecutor>
class Lazy : public Next<ResultType, Executor> {
private:
  using LazyBase = Next<ResultType, Executor>;

protected:
  friend typename LazyBase::Friend;
  class LazyPromiseBase : public LazyBase::NextPromiseBase {
  public:
    using coro_type = Lazy<ResultType, Executor>;
    std::suspend_always initial_suspend() noexcept {
      DEBUG("initial_suspend");
      return {};
    }
  };

public:
  using promise_type = LazyBase::template Promise<LazyPromiseBase>;

  using typename LazyBase::executor_type;
  using typename LazyBase::result_type;
  using awaiter_type = LazyAwaiter<promise_type>;

  static_assert(Awaitable<awaiter_type>, "LazyAwaiter is not Awaitable");

  explicit Lazy(std::coroutine_handle<promise_type> handle) noexcept : _handle(handle) {}

  ~Lazy() { assert(!_handle); }

protected:
  std::coroutine_handle<promise_type> _handle;
};

static_assert(Awaitable<Lazy<int>>, "Lazy<int> is not Awaitable");

XSL_CORO_NE
#endif
