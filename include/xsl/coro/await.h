#pragma once
#ifndef XSL_CORO_AWAIT
#  define XSL_CORO_AWAIT
#  include "xsl/coro/def.h"
#  include "xsl/logctl.h"

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
    DEBUG("");
    return false;
  }
  template <class Promise>
  void await_suspend(std::coroutine_handle<Promise> handle) noexcept {
    DEBUG("");
    _func([this, handle](ResultType&& result) {
      _result = std::move(result);
      DEBUG("set result");
      handle.promise().dispatch([handle]() { handle.resume(); });
    });
  }
  ResultType await_resume() noexcept {
    DEBUG("return result");
    return std::move(*_result);
  }

private:
  std::function<void(std::function<void(ResultType)>)> _func;
  std::optional<ResultType> _result;
};

XSL_CORO_NAMESPACE_END
#endif  // XSL_CORO_AWAIT
