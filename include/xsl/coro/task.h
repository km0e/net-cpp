/**
 * @file task.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Task coroutine
 * @version 0.2
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_CORO_TASK
#  define XSL_CORO_TASK
#  include "xsl/coro/base.h"
#  include "xsl/coro/block.h"
#  include "xsl/coro/chain.h"
#  include "xsl/coro/def.h"
#  include "xsl/coro/detach.h"
#  include "xsl/coro/executor.h"
#  include "xsl/logctl.h"

#  include <cassert>
#  include <coroutine>
#  include <expected>
#  include <type_traits>
#  include <utility>

XSL_CORO_NB

template <class Promise>
class TaskAwaiter {
protected:
  using promise_type = Promise;

public:
  typedef typename promise_type::executor_type executor_type;
  typedef typename promise_type::result_type result_type;

  TaskAwaiter(std::coroutine_handle<promise_type> handle) : _handle(handle) {}
  TaskAwaiter(TaskAwaiter &&another) noexcept : _handle(std::exchange(another._handle, {})) {}
  TaskAwaiter &operator=(TaskAwaiter &&another) noexcept {
    _handle = std::exchange(another._handle, {});
    return *this;
  }
  TaskAwaiter(const TaskAwaiter &) = delete;
  TaskAwaiter &operator=(const TaskAwaiter &) = delete;
  ~TaskAwaiter() {
    LOG7("TaskAwaiter destructor for {}", (uint64_t)_handle.address());
    if (_handle) {
      assert(_handle.done());
      _handle.destroy();
    }
  }
  bool await_ready() const { return false; }

  template <class _Promise>
  std::coroutine_handle<promise_type> await_suspend(std::coroutine_handle<_Promise> handle) {
    LOG7("await_suspend: {} -> {}", (uint64_t)_handle.address(), (uint64_t)handle.address());
    this->_handle.promise().next(handle);
    return this->_handle;
  }

  result_type await_resume() {
    LOG7("task await_resume for {}", (uint64_t)_handle.address());
    return *_handle.promise();
  }

protected:
  std::coroutine_handle<promise_type> _handle;
};

class NextBase {
public:
  NextBase() : _next(nullptr), _executor(nullptr) {}

  std::suspend_always initial_suspend() const noexcept { return {}; }

  struct final_awaiter {
    bool await_ready() const noexcept { return false; }
    std::coroutine_handle<> await_suspend(std::coroutine_handle<>) const noexcept {
      LOG7("final_awaiter await_suspend");
      return _next;
    }
    void await_resume() const noexcept {}
    std::coroutine_handle<> _next;
  };

  final_awaiter final_suspend() const noexcept {
    LOG7("final_suspend");
    return {this->_next};
  }

  const std::shared_ptr<ExecutorBase> &executor() const noexcept { return _executor; }

  template <class _Executor>
  void by(this auto &&self, _Executor &&executor) {
    self._executor = std::forward<_Executor>(executor);
  }

  template <class Promise>
  void next(std::coroutine_handle<Promise> handle) {
    LOG7("Promise next");
    this->_next = handle;
    if constexpr (!std::is_same_v<typename Promise::executor_type, void>) {
      if (!this->executor()) {
        this->_executor = handle.promise().executor();
      }
    }
  }

  template <class Promise>
  void resume(std::coroutine_handle<Promise> handle) {
    if (this->executor()) {
      this->_executor->schedule([handle] mutable {
        LOG7("task resume {}", (uint64_t)handle.address());
        handle();
      });
    } else {
      handle();
    }
  }

protected:
  std::coroutine_handle<> _next;

  std::shared_ptr<ExecutorBase> _executor;
};

template <class ResultType>
class Task;

template <class ResultType>
class TaskPromiseBase : public NextBase, public PromiseBase<ResultType> {
public:
  using coro_type = Task<ResultType>;
  using executor_type = ExecutorBase;
};

template <class ResultType>
class Task {
public:
  using result_type = ResultType;
  using promise_type = Promise<TaskPromiseBase<ResultType>>;
  using awaiter_type = TaskAwaiter<promise_type>;

protected:
  using Friend = HandleControl;
  friend Friend;

  std::coroutine_handle<promise_type> _handle;

  std::coroutine_handle<promise_type> move_handle() { return std::exchange(this->_handle, {}); }

public:
  constexpr Task(std::coroutine_handle<promise_type> handle) noexcept : _handle(handle) {}
  constexpr Task(Task &&task) noexcept : _handle(task.move_handle()) {}
  constexpr Task &operator=(Task &&task) noexcept {
    _handle = task.move_handle();
    return *this;
  }
  constexpr ~Task() { assert(!_handle); }

  awaiter_type operator co_await(this Task &&self) {
    LOG7("move handle to Awaiter");
    return self.move_handle();
  }

  auto transform(this Task &&self, std::invocable<result_type> auto &&f) {
    return ChainAwaiter<awaiter_type>(self.move_handle()).transform(std::forward<decltype(f)>(f));
  }
  /**
   * @brief Block the task
   *
   * @tparam Self
   * @param self
   * @return result_type
   */
  template <class Self>
  result_type block(this Self &&self) {
    LOG7("Task block");
    return _coro::block(std::forward<Self>(self));
  }
  /**
   * @brief Set the executor
   *
   * @tparam Self the task
   * @tparam E the executor
   * @param self the task
   * @param executor the executor
   * @return Self
   */
  template <class Self, class E>
    requires std::constructible_from<std::shared_ptr<ExecutorBase>, E>
  auto &&by(this Self &&self, E &&executor) {
    self._handle.promise().by(std::forward<E>(executor));
    return std::forward<Self>(self);
  }
  /**
   * @brief Detach the task
   *
   * @param self the task, must be rvalue reference
   */
  void detach(this Task &&self) {
    LOG6("task detach");
    _coro::detach(std::move(self));
  }
  /**
   * @brief Detach the task
   *
   * @tparam E
   * @param self the task, must be rvalue reference
   * @param executor the executor
   * @return void
   */
  template <class E>
    requires std::constructible_from<std::shared_ptr<ExecutorBase>, E>
  void detach(this Task &&self, E &&executor) {
    std::move(self).by(std::forward<E>(executor)).detach();
  }
};
XSL_CORO_NE
#endif  // XSL_CORO_TASK
