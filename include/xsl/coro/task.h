/**
 * @file task.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Task coroutine
 * @version 0.21
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
#  include "xsl/coro/def.h"
#  include "xsl/coro/detach.h"
#  include "xsl/coro/executor.h"
#  include "xsl/coro/then.h"
#  include "xsl/logctl.h"

#  include <cassert>
#  include <concepts>
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

  constexpr TaskAwaiter(std::coroutine_handle<promise_type> handle) : _handle(handle) {}
  constexpr TaskAwaiter(TaskAwaiter &&another) noexcept
      : _handle(std::exchange(another._handle, {})) {}
  constexpr TaskAwaiter &operator=(TaskAwaiter &&another) noexcept {
    _handle = std::exchange(another._handle, {});
    return *this;
  }
  constexpr TaskAwaiter(const TaskAwaiter &) = delete;
  constexpr TaskAwaiter &operator=(const TaskAwaiter &) = delete;
  constexpr ~TaskAwaiter() {
    LOG7("TaskAwaiter destructor for {}", (uint64_t)_handle.address());
    if (_handle) {
      assert(_handle.done());
      _handle.destroy();
    }
  }
  constexpr bool await_ready() const { return false; }

  template <class _Promise>
  constexpr std::coroutine_handle<promise_type> await_suspend(
      std::coroutine_handle<_Promise> handle) {
    LOG7("await_suspend: {} -> {}", (uint64_t)_handle.address(), (uint64_t)handle.address());
    this->_handle.promise().next(handle);
    return this->_handle;
  }

  constexpr result_type await_resume() {
    LOG7("task await_resume for {}", (uint64_t)_handle.address());
    return *_handle.promise();
  }

protected:
  std::coroutine_handle<promise_type> _handle;
};

class NextBase {
public:
  constexpr NextBase() : _next(nullptr), _executor(nullptr) {}

  constexpr std::suspend_always initial_suspend() const noexcept { return {}; }

  struct final_awaiter {
    constexpr bool await_ready() const noexcept { return false; }
    constexpr std::coroutine_handle<> await_suspend(std::coroutine_handle<>) const noexcept {
      return _next;
    }
    constexpr void await_resume() const noexcept {}
    std::coroutine_handle<> _next;
  };

  constexpr final_awaiter final_suspend() const noexcept { return {this->_next}; }

  constexpr const std::shared_ptr<ExecutorBase> &executor() const noexcept { return _executor; }

  template <class Awaiter>
    requires(!std::is_reference_v<Awaiter>) && requires(Awaiter &&awaiter) {
      { detach(std::forward<Awaiter>(awaiter)) };
    }
  constexpr std::suspend_never yield_value(Awaiter &&awaiter) {
    _coro::detach(std::forward<Awaiter>(awaiter), this->executor());
    return {};
  }

  constexpr void by(this auto &&self,
                    std::convertible_to<std::shared_ptr<ExecutorBase>> auto &&executor) {
    self._executor = std::forward<decltype(executor)>(executor);
  }

  template <class Promise>
  constexpr void next(std::coroutine_handle<Promise> handle) {
    LOG7("Promise next");
    this->_next = handle;
    if constexpr (!std::is_same_v<typename Promise::executor_type, void>) {
      if (!this->executor()) {
        LOG6("set executor");
        this->_executor = handle.promise().executor();
      }
    }
  }

  template <class Promise>
  constexpr void resume(std::coroutine_handle<Promise> handle) {
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
  std::coroutine_handle<promise_type> _handle;

  constexpr std::coroutine_handle<promise_type> move_handle() && {
    return std::exchange(this->_handle, {});
  }

public:
  constexpr Task(std::coroutine_handle<promise_type> handle) noexcept : _handle(handle) {}
  constexpr Task(Task &&task) noexcept : _handle(std::move(task).move_handle()) {}
  constexpr Task &operator=(Task &&task) noexcept {
    _handle = task.move_handle();
    return *this;
  }
  constexpr ~Task() { assert(!_handle); }

  constexpr awaiter_type operator co_await(this Task &&self) {
    LOG7("move handle to Awaiter");
    return std::move(self).move_handle();
  }

  constexpr auto then(this Task &&self, std::invocable<result_type> auto &&f) {
    return ThenAwaiter<awaiter_type>(std::move(self).move_handle())
        .then(std::forward<decltype(f)>(f));
  }
  /**
   * @brief Block the task
   *
   * @tparam Self
   * @param self
   * @return result_type
   */
  constexpr result_type block(this Task &&self) {
    LOG7("Task block");
    return _coro::block(std::move(self));
  }
  /**
   * @brief Set the executor
   *
   * @tparam E the executor type
   * @param self the task, must be rvalue reference
   * @param executor the executor
   * @return requires constexpr&&
   */
  constexpr auto &&by(this auto &&self,
                      std::convertible_to<std::shared_ptr<ExecutorBase>> auto &&executor) {
    self._handle.promise().by(std::forward<decltype(executor)>(executor));
    return std::forward<decltype(self)>(self);
  }
  /**
   * @brief Detach the task
   *
   * @param self the task, must be rvalue reference
   */
  constexpr void detach(this Task &&self) {
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
  constexpr void detach(this Task &&self,
                        std::convertible_to<std::shared_ptr<ExecutorBase>> auto &&executor) {
    _coro::detach(std::move(self), std::forward<decltype(executor)>(executor));
  }
};
XSL_CORO_NE
#endif  // XSL_CORO_TASK
