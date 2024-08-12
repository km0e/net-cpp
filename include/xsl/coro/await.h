#pragma once
#ifndef XSL_CORO_AWAIT
#  define XSL_CORO_AWAIT
#  include "xsl/coro/def.h"
#  include "xsl/coro/executor.h"
#  include "xsl/wheel/utils.h"

#  include <coroutine>
#  include <memory>

XSL_CORO_NB

template <class Executor = ExecutorBase>
class GetExecutor {
public:
  using executor_type = Executor;
  GetExecutor() : _executor() {}

  bool await_ready() const noexcept { return true; }

  template <class _Promise>
  bool await_suspend(std::coroutine_handle<_Promise>) noexcept {
    wheel::dynamic_assert(false, "GetExecutor::await_suspend should not be called");
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
