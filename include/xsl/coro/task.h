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
#  include "xsl/coro/guard.h"
#  include "xsl/coro/then.h"
#  include "xsl/logctl.h"
#  include "xsl/type_traits.h"

#  include <cassert>
#  include <concepts>
#  include <coroutine>
#  include <expected>
#  include <tuple>
#  include <type_traits>
#  include <utility>

XSL_CORO_NB

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

  template <class Awaiter, class... _Args>
    requires(!std::is_reference_v<Awaiter>)
  constexpr std::suspend_never yield_value(Awaiter &&awaiter, _Args &&...args) {
    if constexpr (sizeof...(args) == 0) {
      _coro::detach(std::forward<Awaiter>(awaiter), this->executor());
    } else {
      ArgGuard creator(
          std::forward_as_tuple(std::forward<Awaiter>(awaiter), std::forward<_Args>(args)...));
      _coro::detach(std::move(creator), this->executor());
    }
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
protected:
  using PromiseBase<ResultType>::_result;

public:
  using coro_type = Task<ResultType>;
  using executor_type = ExecutorBase;
};

template <class ResultType>
class Task {
public:
  using result_type = ResultType;
  using executor_type = ExecutorBase;
  using promise_type = Promise<TaskPromiseBase<ResultType>>;

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
  constexpr ~Task() {
    if (_handle) {
      assert(_handle.done());
      _handle.destroy();
    }
  }

  template <not_reference Self>
  constexpr Self operator co_await(this Self &&self) {
    LOG7("move handle to Awaiter");
    return std::move(self);
  }

  template <class Func>
    requires((std::same_as<void, result_type> && std::invocable<Func>)
             || std::invocable<Func, result_type &&>)
  constexpr auto then(this Task &&self, Func &&f) {
    return ThenAwaiter<Task>(std::move(self).move_handle()).then(std::forward<decltype(f)>(f));
  }
  /**
   * @brief Block the task
   *
   * @tparam Self
   * @param self
   * @return result_type
   */
  template <not_reference Self>
  constexpr decltype(auto) block(this Self &&self) {
    LOG7("Task block");
    return _coro::block(std::move(self));
  }
  /**
   * @brief Block the task
   *
   * @param self
   * @param executor the executor
   * @return auto&&
   */
  constexpr auto &&by(this auto &&self,
                      std::convertible_to<std::shared_ptr<ExecutorBase>> auto &&executor) {
    self._handle.promise().by(std::forward<decltype(executor)>(executor));
    return std::forward<decltype(self)>(self);
  }
  /// @brief Detach the task
  template <not_reference Self>
  constexpr void detach(this Self &&self) {
    LOG6("task detach");
    _coro::detach(std::move(self));
  }
  /// @brief Detach the task with executor
  template <not_reference Self>
  constexpr void detach(this Self &&self,
                        std::convertible_to<std::shared_ptr<ExecutorBase>> auto &&executor) {
    _coro::detach(std::move(self), std::forward<decltype(executor)>(executor));
  }

public:
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
};
XSL_CORO_NE
#endif  // XSL_CORO_TASK
