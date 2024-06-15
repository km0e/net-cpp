#pragma once
#ifndef XSL_CORO_TASK
#  define XSL_CORO_TASK
#  include "xsl/coro/block.h"
#  include "xsl/coro/def.h"
#  include "xsl/coro/executor.h"
#  include "xsl/coro/final.h"
#  include "xsl/coro/result.h"

#  include <spdlog/spdlog.h>

#  include <cassert>
#  include <coroutine>
#  include <exception>
#  include <memory>
#  include <optional>
#  include <utility>

XSL_CORO_NAMESPACE_BEGIN

template <typename ResultType, typename Executor = NoopExecutor>
class Task;

template <class ResultType, class Executor>
class TaskAwaiter {
public:
  using promise_type = typename Task<ResultType, Executor>::promise_type;

  TaskAwaiter(std::coroutine_handle<promise_type> handle) : _handle(handle) {}
  TaskAwaiter(TaskAwaiter &&another) noexcept : _handle(std::exchange(another._handle, {})) {}
  TaskAwaiter &operator=(TaskAwaiter &&another) noexcept {
    _handle = std::exchange(another._handle, {});
    return *this;
  }
  TaskAwaiter(const TaskAwaiter &) = delete;
  TaskAwaiter &operator=(const TaskAwaiter &) = delete;
  ~TaskAwaiter() {
    assert(_handle.done());
    _handle.destroy();
  }
  bool await_ready() const {
    _handle();
    return _handle.done();
  }

  template <class Promise>
  void await_suspend(std::coroutine_handle<Promise> handle) {
    if constexpr (!std::is_same_v<typename promise_traits<Promise>::executor_type, Executor>) {
      this->_handle.promise().next(this->_handle);
    } else {
      if (handle.promise().executor()) {
        this->_handle.promise().next(this->_handle);
      } else if (auto executor = handle.promise().executor(); executor) {
        this->_handle.promise().by(executor).next(this->_handle);
      } else {
        this->_handle.promise().next(this->_handle);
      }
    }
  }

  ResultType await_resume() { return *_handle.promise(); }

private:
  std::coroutine_handle<promise_type> _handle;
};

template <typename ResultType, typename Executor>
class TaskPromiseBase {
public:
  using result_type = ResultType;
  using executor_type = Executor;
  TaskPromiseBase() : _result(std::nullopt), _next_handle(nullptr), _executor(nullptr) {}
  std::suspend_always initial_suspend() {
    SPDLOG_DEBUG("initial_suspend");
    return {};
  }

  std::suspend_always final_suspend() noexcept {
    SPDLOG_DEBUG("final_suspend");
    if (!_next_handle) {
      return {};
    }
    if (this->_executor)
      this->_executor->schedule([next_handle = _next_handle]() mutable { next_handle(); });
    else
      _next_handle();
    return {};
  }

  void unhandled_exception() { _result = Result<ResultType>(std::current_exception()); }

  ResultType operator*() {
    SPDLOG_DEBUG("operator*");
    return _result->get_or_throw();
  }

  auto &by(const std::shared_ptr<Executor> &executor) {
    this->_executor = executor;
    return *this;
  }

  void next(std::coroutine_handle<> handle) { _next_handle = handle; }

  const std::shared_ptr<Executor> &executor() const { return _executor; }

  template <typename F>
    requires std::invocable<F>
  void dispatch(F &&f) {
    if (this->executor()) {
      this->_executor->schedule(std::move(f));
    } else {
      f();
    }
  }

protected:
  std::optional<Result<ResultType>> _result;

  std::coroutine_handle<> _next_handle;

  std::shared_ptr<Executor> _executor;
};

template <typename ResultType, typename Executor>
class Task {
public:
  class TaskPromise : public TaskPromiseBase<ResultType, Executor> {
    using Base = TaskPromiseBase<ResultType, Executor>;
    using Base::_result;

  public:
    Task<ResultType, Executor> get_return_object() {
      SPDLOG_DEBUG("get_return_object");
      return Task{std::coroutine_handle<TaskPromise>::from_promise(*this)};
    }

    void return_value(ResultType value) {
      SPDLOG_DEBUG("return_value");
      _result = Result<ResultType>(std::move(value));
    }
  };

  using promise_type = TaskPromise;

  explicit Task(std::coroutine_handle<promise_type> handle) noexcept : _handle(handle) {}

  Task(Task &&task) noexcept : _handle(std::exchange(task._handle, {})) {}

  Task &operator=(Task &&another) noexcept {
    _handle = std::exchange(another._handle, {});
    return *this;
  }
  Task(Task &) = delete;

  Task &operator=(Task &) = delete;

  ~Task() { assert(!_handle); }  // task should be moved to Final/Block

  Task &by(std::shared_ptr<Executor> &executor) {
    this->_handle.promise().by(executor);
    return *this;
  }

  void detach() { this->_handle.promise().dispatch(final(std::move(*this))); }

  void detach(std::shared_ptr<Executor> &executor) { this->by(executor).detach(); }

  ResultType block() { return *coro::block(std::move(*this)); }

  TaskAwaiter<ResultType, Executor> operator co_await() { return std::exchange(_handle, {}); }

private:
  std::coroutine_handle<promise_type> _handle;
};

template <typename Executor>
class Task<void, Executor> {
public:
  class TaskPromise : public TaskPromiseBase<void, Executor> {
    using Base = TaskPromiseBase<void, Executor>;
    using Base::_result;

  public:
    Task<void, Executor> get_return_object() {
      return Task{std::coroutine_handle<TaskPromise>::from_promise(*this)};
    }

    void return_void() { _result = Result<void>(); }
  };

  using promise_type = TaskPromise;

  explicit Task(std::coroutine_handle<promise_type> handle) noexcept : _handle(handle) {}

  Task(Task &&task) noexcept : _handle(std::exchange(task._handle, {})) {}

  Task &operator=(Task &&another) noexcept {
    _handle = std::exchange(another._handle, {});
    return *this;
  }
  Task(Task &) = delete;

  Task &operator=(Task &) = delete;

  ~Task() { assert(!_handle); }  // task should be moved to Final/Block

  Task &by(std::shared_ptr<Executor> &executor) {
    this->_handle.promise().by(executor);
    return *this;
  }

  void detach() { this->_handle.promise().dispatch(final(std::move(*this))); }

  void detach(std::shared_ptr<Executor> &executor) { this->by(executor).detach(); }

  void block() { *coro::block(std::move(*this)); }

  TaskAwaiter<void, Executor> operator co_await() {
    SPDLOG_DEBUG("move handle to TaskAwaiter");
    return std::exchange(_handle, {});
  }

private:
  std::coroutine_handle<promise_type> _handle;
};

XSL_CORO_NAMESPACE_END
#endif  // XSL_CORO_TASK
