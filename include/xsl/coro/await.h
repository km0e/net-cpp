#pragma once
#ifndef XSL_CORO_AWAIT
#  define XSL_CORO_AWAIT
#  include "xsl/coro/def.h"
#  include "xsl/coro/executor.h"

#  include <coroutine>
#  include <memory>

XSL_CORO_NAMESPACE_BEGIN

template <typename Awaiter, typename R = typename Awaiter::ResultType,
          typename Executor = NoopExecutor>
class AwaiterProxy {
public:
  using ResultType = R;
  AwaiterProxy(std::shared_ptr<Executor> executor, Awaiter awaiter)
      : _executor(executor), _awaiter(awaiter) {}

  void resume() {
    _executor->execute([this]() { this->_handle.resume(); });
  }

private:
  std::shared_ptr<Executor> _executor;
  Awaiter _awaiter;
  std::coroutine_handle<> _handle;
};

// template <typename Awaiter, typename R = typename Awaiter::ResultType,
//           typename Executor = NoopExecutor>
// class AwaiterProxy {
// public:
//   using ResultType = R;
//   AwaiterProxy(std::shared_ptr<Executor> executor, Awaiter awaiter)
//       : _executor(executor), _awaiter(awaiter) {}

//   bool await_ready() const { return _awaiter.await_ready(); }

//   void await_suspend(std::coroutine_handle<> handle) { _awaiter.await_suspend(handle); }

//   R await_resume() {
//     this->_executor->execute([this]() { this->_awaiter.resume(); });
//   }

// private:
//   std::shared_ptr<Executor> _executor;
//   Awaiter _awaiter;
// };
XSL_CORO_NAMESPACE_END
#endif  // XSL_CORO_AWAIT
