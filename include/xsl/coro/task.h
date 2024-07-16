#pragma once
#ifndef XSL_CORO_TASK
#  define XSL_CORO_TASK
#  include "xsl/coro/base.h"
#  include "xsl/coro/def.h"
#  include "xsl/coro/executor.h"
#  include "xsl/logctl.h"

#  include <cassert>
#  include <coroutine>
#  include <expected>

XSL_CORO_NB

template <class Promise>
class TaskAwaiter : public AwaiterBase<Promise> {
  using Base = AwaiterBase<Promise>;

public:
  using promise_type = typename Base::promise_type;
  using result_type = typename Base::result_type;
  using Base::Base;
  bool await_ready() const { return _handle.done(); }

protected:
  using Base::_handle;
};

template <typename ResultType, typename Executor = NoopExecutor>
class Task : public CoroBase<ResultType, Executor> {
  friend class CoroBase<ResultType, Executor>;
  using TaskBase = CoroBase<ResultType, Executor>;

  class TaskPromiseBase : public TaskBase::PromiseBase {
    using Base = TaskBase::PromiseBase;

  public:
    using coro_type = Task<ResultType, Executor>;
    std::suspend_never initial_suspend() noexcept {
      DEBUG("initial_suspend");
      return {};
    }
  };

public:
  using promise_type = TaskBase::template Promise<TaskPromiseBase>;

  using typename TaskBase::executor_type;
  using typename TaskBase::result_type;
  using awaiter_type = TaskAwaiter<promise_type>;

  static_assert(Awaitable<awaiter_type>, "TaskAwaiter is not Awaitable");

  explicit Task(std::coroutine_handle<promise_type> handle) noexcept : _handle(handle) {}

protected:
  std::coroutine_handle<promise_type> _handle;
};

static_assert(Awaitable<Task<int>>, "Task<int> is not Awaitable");

XSL_CORO_NE
#endif  // XSL_CORO_TASK
