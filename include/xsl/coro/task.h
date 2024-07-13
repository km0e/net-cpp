#pragma once
#ifndef XSL_CORO_TASK
#  define XSL_CORO_TASK
#  include "xsl/coro/block.h"
#  include "xsl/coro/def.h"
#  include "xsl/coro/executor.h"
#  include "xsl/coro/final.h"
#  include "xsl/logctl.h"

#  include <cassert>
#  include <concepts>
#  include <coroutine>
#  include <cstdint>
#  include <exception>
#  include <expected>
#  include <functional>
#  include <memory>
#  include <optional>
#  include <type_traits>
#  include <utility>

XSL_CORO_NB

template <typename ResultType, typename Executor = NoopExecutor>
class Task;

template <class ResultType, class Executor>
class TaskAwaiter {
public:
  using promise_type = typename Task<ResultType, Executor>::promise_type;
  using result_type = typename promise_traits<promise_type>::result_type;

  TaskAwaiter(std::coroutine_handle<promise_type> handle) : _handle(handle) {}
  TaskAwaiter(TaskAwaiter &&another) noexcept : _handle(std::exchange(another._handle, {})) {}
  TaskAwaiter &operator=(TaskAwaiter &&another) noexcept {
    _handle = std::exchange(another._handle, {});
    return *this;
  }
  TaskAwaiter(const TaskAwaiter &) = delete;
  TaskAwaiter &operator=(const TaskAwaiter &) = delete;
  ~TaskAwaiter() {
    DEBUG("TaskAwaiter destructor for {}", (uint64_t)_handle.address());
    if (_handle) {
      assert(_handle.done());
      _handle.destroy();
    }
  }

  bool await_ready() const { return _handle.done(); }

  template <class Promise>
  void await_suspend(std::coroutine_handle<Promise> handle) {
    DEBUG("task await_suspend for {}", (uint64_t)handle.address());
    if constexpr (!std::is_same_v<typename promise_traits<Promise>::executor_type, Executor>) {
      this->_handle.promise().next(handle);
    } else {
      if (this->_handle.promise().executor()) {
        this->_handle.promise().next(handle);
      } else if (auto executor = handle.promise().executor(); executor) {
        this->_handle.promise().by(executor).next(handle);
      } else {
        this->_handle.promise().next(handle);
      }
    }
  }

  result_type await_resume() {
    DEBUG("task await_resume for {}", (uint64_t)_handle.address());
    return *_handle.promise();
  }

private:
  std::coroutine_handle<promise_type> _handle;
};

template <typename ResultType, typename Executor>
class Task {
  class TaskPromiseBase {
  public:
    using result_type = ResultType;
    using executor_type = Executor;
    TaskPromiseBase() : _result(std::nullopt), _next_resume(std::nullopt), _executor(nullptr) {}

    auto get_return_object(this auto &&self) {
      DEBUG("get_return_object");
      return Task<result_type, executor_type>{
          std::coroutine_handle<std::remove_cvref_t<decltype(self)>>::from_promise(self)};
    }

    std::suspend_never initial_suspend() noexcept {
      DEBUG("initial_suspend");
      return {};
    }

    std::suspend_always final_suspend() noexcept {
      DEBUG("final_suspend");
      return {};
    }

    void unhandled_exception() { _result = std::unexpected{std::current_exception()}; }

    result_type operator*() {
      DEBUG("operator*");
      if (*_result) {
        if constexpr (std::is_same_v<result_type, void>) {
          return **_result;
        } else {
          return std::move(**_result);
        }
      }
      std::rethrow_exception(_result->error());
    }

    auto &by(const std::shared_ptr<Executor> &executor) {
      this->_executor = executor;
      return *this;
    }
    template <class Promise>
    void next(std::coroutine_handle<Promise> handle) {
      _next_resume = [handle]() mutable { handle.promise().resume(handle); };
    }

    template <class Promise>
    void resume(std::coroutine_handle<Promise> handle) {
      this->dispatch([handle, next_resume = std::move(_next_resume)]() mutable {
        handle();
        if (handle.done()) {
          DEBUG("handle is done");
          if (next_resume) {
            DEBUG("next resume");
            (*next_resume)();
          }
        }
      });
    }

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
    std::optional<Result<result_type>> _result;

    std::optional<std::function<void()>> _next_resume;

    std::shared_ptr<Executor> _executor;
  };

  using PromiseBase = TaskPromiseBase;

  template <typename _ResultType, typename _Executor>
  class TaskPromise : public Task<_ResultType, _Executor>::PromiseBase {
    using Base = Task<_ResultType, _Executor>::PromiseBase;
    using Base::_result;

  public:
    using result_type = typename Base::result_type;
    void return_value(result_type &&value) {
      DEBUG("return_value");
      _result = Result<result_type>(std::move(value));
    }
  };

  template <typename _Executor>
  class TaskPromise<void, _Executor> : public Task<void, _Executor>::PromiseBase {
    using Base = Task<void, _Executor>::PromiseBase;
    using Base::_result;

  public:
    using result_type = typename Base::result_type;
    void return_void() { _result = Result<void>(); }
  };

public:
  using promise_type = TaskPromise<ResultType, Executor>;
  using result_type = promise_traits<promise_type>::result_type;
  using executor_type = promise_traits<promise_type>::executor_type;

  explicit Task(std::coroutine_handle<promise_type> handle) noexcept : _handle(handle) {}

  Task(Task &&task) noexcept : _handle(std::exchange(task._handle, {})) {}

  Task &operator=(Task &&another) noexcept {
    _handle = std::exchange(another._handle, {});
    return *this;
  }
  Task(const Task &) = delete;

  Task &operator=(const Task &) = delete;

  ~Task() { assert(!_handle); }  // task should be moved to Final/TaskAwaiter

  auto &&by(this auto &&self, std::shared_ptr<executor_type> &executor) {
    static_cast<Task *>(&self)->_handle.promise().by(executor);
    return std::forward<decltype(self)>(self);
  }

  void detach() {
    DEBUG("task detach");
    this->_handle.promise().dispatch(
        [awaiter = std::move(this->_co_await())]() mutable -> Final<result_type> {
          co_await awaiter;
        });
  }

  void detach(std::shared_ptr<executor_type> &executor) { this->by(executor).detach(); }

  result_type block() { return coro::block(std::move(*this)); }

  TaskAwaiter<result_type, executor_type> operator co_await() {
    DEBUG("move handle to TaskAwaiter");
    return this->_co_await();
  }

protected:
  std::coroutine_handle<promise_type> _handle;

  TaskAwaiter<result_type, executor_type> _co_await() { return std::exchange(_handle, {}); }
};

XSL_CORO_NE
#endif  // XSL_CORO_TASK
