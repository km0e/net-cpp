#pragma once
#ifndef XSL_CORO_AWAIT
#  define XSL_CORO_AWAIT
#  include "xsl/coro/def.h"
#  include "xsl/logctl.h"

#  include <coroutine>
#  include <functional>
#  include <memory>

XSL_CORO_NB

template <class Executor>
class GetExecutor {
public:
  using executor_type = Executor;
  GetExecutor() : _executor() {}

  bool await_ready() const noexcept { return false; }

  template <class Promise>
    requires std::is_same_v<executor_type, typename Promise::executor_type>
  bool await_suspend(std::coroutine_handle<Promise> handle) noexcept {
    LOG5("GetExecutor await_suspend for {}", (uint64_t)handle.address());
    _executor = handle.promise().executor();
    return false;
  }

  std::shared_ptr<executor_type> await_resume() noexcept { return std::move(_executor); }

protected:
  std::shared_ptr<executor_type> _executor;
};

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
    LOG5("CallbackAwaiter await_suspend for {}", (uint64_t)handle.address());
    _func([this, handle](Ntf&& result) {
      _ntf = std::move(result);
      LOG5("CallbackAwaiter set result");
      handle.promise().resume(handle);
    });
  }
  ResultType
  await_resume() noexcept {  // if ResultType is not Ntf, then you should override this function
    LOG5("CallbackAwaiter return result");
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
    LOG5("await_ready");
    return false;
  }
  template <class Promise>
  void await_suspend(std::coroutine_handle<Promise> handle) noexcept {
    LOG5("callback await_suspend for {}", (uint64_t)handle.address());
    _func([this, handle]() {
      DEBUG("set result");
      handle.promise().dispatch([handle]() {
        handle();
        if (handle.done()) {
          DEBUG("handle is done");
          handle.promise().next();
        }
      });
    });
  }
  ResultType
  await_resume() noexcept {  // if ResultType is not Ntf, then you should override this function
    DEBUG("callback await_resume");
    return;
  }

protected:
  callback_type _func;
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
      handle.promise().dispatch([handle]() {
        handle();
        if (handle.done()) {
          DEBUG("handle is done");
          handle.promise().next();
        }
      });
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
    DEBUG("callback await_suspend for {}", (uint64_t)handle.address());
    _func([this, handle]() {
      DEBUG("set result");
      handle.promise().resume(handle);
    });
  }
  ResultType
  await_resume() noexcept {  // if ResultType is not Ntf, then you should override this function
    LOG5("callback await_resume");
    return;
  }

protected:
  callback_type _func;
};

XSL_CORO_NE
#endif  // XSL_CORO_AWAIT
