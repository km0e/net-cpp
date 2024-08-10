#pragma once
#ifndef XSL_CORO_AWAIT
#  define XSL_CORO_AWAIT
#  include "xsl/coro/def.h"
#  include "xsl/coro/executor.h"
#  include "xsl/logctl.h"

#  include <coroutine>
#  include <memory>

XSL_CORO_NB

template <class Executor = ExecutorBase>
class GetExecutor {
public:
  using executor_type = Executor;
  GetExecutor() : _executor() {}

  bool await_ready() const noexcept { return false; }

  template <class Promise>
    requires std::is_same_v<executor_type, typename Promise::executor_type>
  bool await_suspend(std::coroutine_handle<Promise> handle) noexcept {
    LOG6("GetExecutor await_suspend for {}", (uint64_t)handle.address());
    _executor = handle.promise().executor();
    return false;
  }

  std::shared_ptr<executor_type> await_resume() noexcept { return std::move(_executor); }

protected:
  std::shared_ptr<executor_type> _executor;
};

XSL_CORO_NE
#endif  // XSL_CORO_AWAIT
