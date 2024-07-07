#pragma once
#ifndef XSL_CORO_AWAIT
#  define XSL_CORO_AWAIT
#  include "xsl/coro/def.h"
#  include "xsl/logctl.h"

#  include <coroutine>
#  include <functional>

XSL_CORO_NAMESPACE_BEGIN

template <class Ntf, class ResultType = Ntf>
class CallbackAwaiter {
public:
  using callback_type = std::function<void(std::function<void(Ntf&&)>&&)>;
  CallbackAwaiter(callback_type&& func) : _func(std::move(func)), _ntf() {}
  bool await_ready() const noexcept {
    DEBUG("");
    return false;
  }
  template <class Promise>
  void await_suspend(std::coroutine_handle<Promise> handle) noexcept {
    DEBUG("");
    _func([this, handle](Ntf&& result) {
      _ntf = std::move(result);
      DEBUG("set result");
      handle.promise().dispatch([handle]() { handle.resume(); });
    });
  }
  ResultType await_resume() noexcept {
    DEBUG("return result");
    return std::move(*_ntf);
  }

protected:
  callback_type _func;
  std::optional<Ntf> _ntf;
};

XSL_CORO_NAMESPACE_END
#endif  // XSL_CORO_AWAIT
