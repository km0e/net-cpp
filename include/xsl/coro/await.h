#pragma once
#ifndef XSL_CORO_AWAIT
#  define XSL_CORO_AWAIT
#  include "xsl/coro/def.h"

#  include <spdlog/spdlog.h>

#  include <coroutine>
#  include <functional>
#  include <optional>

XSL_CORO_NAMESPACE_BEGIN

template <typename ResultType>
class CallbackAwaiter {
public:
  CallbackAwaiter(std::function<void(std::function<void(ResultType)>)> func)
      : _func(func), _result(std::nullopt) {}
  bool await_ready() const noexcept {
    SPDLOG_DEBUG("");
    return false;
  }
  template <class Promise>
  void await_suspend(std::coroutine_handle<Promise> handle) noexcept {
    SPDLOG_DEBUG("");
    _func([this, handle](ResultType result) {
      _result = result;
      SPDLOG_DEBUG("result: {}", *_result);
      handle.promise().dispatch([handle]() { handle.resume(); });
    });
  }
  ResultType await_resume() noexcept {
    SPDLOG_DEBUG("");
    return *_result;
  }

private:
  std::function<void(std::function<void(ResultType)>)> _func;
  std::optional<ResultType> _result;
};

XSL_CORO_NAMESPACE_END
#endif  // XSL_CORO_AWAIT
