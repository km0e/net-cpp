#pragma once
#ifndef XSL_CORO_TASK
#  define XSL_CORO_TASK
#  include "xsl/coro/def.h"
#  include "xsl/coro/executor.h"
#  include "xsl/coro/next.h"
#  include "xsl/logctl.h"

#  include <cassert>
#  include <coroutine>
#  include <expected>

XSL_CORO_NB

template <class Promise>
class TaskAwaiter : public Awaiter<Promise> {
private:
  using Base = Awaiter<Promise>;

protected:
  using promise_type = Promise;

public:
  using typename Base::result_type;

  using Base::Base;
  bool await_ready() const { return _handle.done(); }

protected:
  using Base::_handle;
};

template <class ResultType, class Executor = NoopExecutor>
class Task;

template <class ResultType, class Executor>
class TaskPromiseBase : public NextPromiseBase<ResultType, Executor> {
public:
  using coro_type = Task<ResultType, Executor>;
  std::suspend_never initial_suspend() noexcept {
    LOG6("initial_suspend");
    return {};
  }
};

template <class ResultType, class Executor>
class Task : public Next<ResultType, Executor> {
private:
  using Base = Next<ResultType, Executor>;

protected:
  friend typename Base::Friend;

public:
  using promise_type = Promise<TaskPromiseBase<ResultType, Executor>>;

  using typename Base::executor_type;
  using typename Base::result_type;
  using awaiter_type = TaskAwaiter<promise_type>;

  static_assert(Awaitable<awaiter_type>, "TaskAwaiter is not Awaitable");

  explicit Task(std::coroutine_handle<promise_type> handle) noexcept : _handle(handle) {}

  Task(Task &&task) noexcept : _handle(std::exchange(task._handle, {})) {}

  ~Task() { assert(!_handle); }

protected:
  std::coroutine_handle<promise_type> _handle;
};

static_assert(Awaitable<Task<int>>, "Task<int> is not Awaitable");

XSL_CORO_NE
#endif  // XSL_CORO_TASK
