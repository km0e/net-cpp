/**
 * @file await.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Await for coroutine
 * @version 0.1
 * @date 2024-09-01
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_CORO_AWAIT
#  define XSL_CORO_AWAIT
#  include "xsl/coro/def.h"
#  include "xsl/coro/executor.h"
#  include "xsl/wheel.h"

#  include <coroutine>
#  include <memory>

XSL_CORO_NB

class GetExecutor {//TODO: remove this class
public:
  using executor_type = ExecutorBase;
  GetExecutor() : _executor() {}

  bool await_ready() const noexcept { return true; }

  template <class _Promise>
  bool await_suspend(std::coroutine_handle<_Promise>) noexcept {
    rt_assert(false, "GetExecutor::await_suspend should not be called");
    return false;
  }

  std::shared_ptr<executor_type> await_resume() noexcept { return std::move(_executor); }

  template <class E>
    requires std::constructible_from<std::shared_ptr<executor_type>, E>
  auto &&by(this auto &&self, E &&executor) {
    self._executor = executor;
    return std::forward<decltype(self)>(self);
  }

protected:
  std::shared_ptr<executor_type> _executor;
};

XSL_CORO_NE
#endif  // XSL_CORO_AWAIT
