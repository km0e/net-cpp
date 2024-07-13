#pragma once
#ifndef XSL_CORO_AWAIT
#  define XSL_CORO_AWAIT
#  include "xsl/coro/def.h"
#  include "xsl/logctl.h"

#  include <coroutine>
#  include <functional>

XSL_CORO_NB

template <class Ntf, class ResultType = Ntf>
class CallbackAwaiter {
public:
  using callback_type = std::function<void(std::function<void(Ntf&&)>&&)>;

  CallbackAwaiter(callback_type&& func) : _func(std::move(func)), _ntf() {}
  bool await_ready() const noexcept {
    DEBUG("CallbackAwaiter await_ready");
    return false;
  }
  template <class Promise>
  void await_suspend(std::coroutine_handle<Promise> handle) noexcept {
    DEBUG("CallbackAwaiter await_suspend for {}", (uint64_t)handle.address());
    _func([this, handle](Ntf&& result) {
      _ntf = std::move(result);
      DEBUG("CallbackAwaiter set result");
      handle.promise().resume(handle);
    });
  }
  ResultType
  await_resume() noexcept {  // if ResultType is not Ntf, then you should override this function
    DEBUG("CallbackAwaiter return result");
    return std::move(*_ntf);
  }

protected:
  callback_type _func;
  std::optional<Ntf> _ntf;
};

template <class ResultType>
class CallbackAwaiter<void, ResultType> {
public:
  using callback_type = std::function<void(std::function<void(void)>&&)>;

  CallbackAwaiter(callback_type&& func) : _func(std::move(func)) {}
  bool await_ready() const noexcept {
    DEBUG("await_ready");
    return false;
  }
  template <class Promise>
  void await_suspend(std::coroutine_handle<Promise> handle) noexcept {
    DEBUG("await_suspend");
    _func([this, handle]() {
      DEBUG("set result");
      handle.promise().resume(handle);
    });
  }
  ResultType
  await_resume() noexcept {  // if ResultType is not Ntf, then you should override this function
    DEBUG("return result");
    return;
  }

protected:
  callback_type _func;
};

XSL_CORO_NE
#endif  // XSL_CORO_AWAIT
