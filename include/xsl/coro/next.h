#pragma once
#ifndef XSL_CORO_NEXT
#  define XSL_CORO_NEXT
#  include "xsl/coro/base.h"
#  include "xsl/coro/block.h"
#  include "xsl/coro/def.h"
#  include "xsl/coro/detach.h"
#  include "xsl/coro/executor.h"
#  include "xsl/logctl.h"

#  include <concepts>
#  include <functional>
#  include <memory>
#  include <optional>
#  include <utility>
XSL_CORO_NB

template <class Promise>
class Awaiter {
protected:
  using promise_type = Promise;

public:
  typedef typename promise_type::executor_type executor_type;
  typedef typename promise_type::result_type result_type;

  Awaiter(std::coroutine_handle<promise_type> handle) : _handle(handle) {}
  Awaiter(Awaiter &&another) noexcept : _handle(std::exchange(another._handle, {})) {}
  Awaiter &operator=(Awaiter &&another) noexcept {
    _handle = std::exchange(another._handle, {});
    return *this;
  }
  Awaiter(const Awaiter &) = delete;
  Awaiter &operator=(const Awaiter &) = delete;
  ~Awaiter() {
    LOG5("TaskAwaiter destructor for {}", (uint64_t)_handle.address());
    if (_handle) {
      assert(_handle.done());
      _handle.destroy();
    }
  }

  template <class _Promise>
  void await_suspend(std::coroutine_handle<_Promise> handle) {
    LOG5("await_suspend: {} -> {}", (uint64_t)_handle.address(), (uint64_t)handle.address());
    if constexpr (std::is_same_v<typename _Promise::executor_type, void>) {
      this->_handle.promise().next(handle);
    } else {
      if (this->_handle.promise().executor()) {
        this->_handle.promise().next(handle);
      } else if (auto executor = handle.promise().executor(); executor) {
        this->_handle.promise().by(executor).next(handle);
      } else {
        this->_handle.promise().next(handle);
      }
    }
  }

  result_type await_resume() {
    LOG5("task await_resume for {}", (uint64_t)_handle.address());
    return *_handle.promise();
  }

protected:
  std::coroutine_handle<promise_type> _handle;
};

template <class ResultType, class Executor = ExecutorBase>
class NextPromiseBase : public PromiseBase<ResultType> {
private:
  using Base = PromiseBase<ResultType>;

public:
  using typename Base::result_type;
  using executor_type = Executor;

  NextPromiseBase() : Base(), _next_resume(std::nullopt), _executor(nullptr) {}

  template <class Promise>
  void resume(std::coroutine_handle<Promise> handle) {
    this->dispatch([handle, this]() mutable {
      LOG5("task resume {}", (uint64_t)handle.address());
      handle();
      LOG5("task resume {} done", (uint64_t)handle.address());
      if (handle.done()) {
        LOG5("task resume handle done");
        if (this->_next_resume) {
          LOG5("task resume next_resume");
          (*this->_next_resume)();
        }
      }
    });
  }

  template <std::invocable F>
  void dispatch(F &&f) {
    if (this->executor()) {
      this->_executor->schedule(std::move(f));
    } else {
      f();
    }
  }

  const std::shared_ptr<Executor> &executor() const noexcept { return _executor; }

  template <class Promise>
  void next(std::coroutine_handle<Promise> handle) {
    _next_resume = [handle]() mutable { handle.promise().resume(handle); };
  }

  template <class E>
    requires std::constructible_from<std::shared_ptr<Executor>, E>
  auto &&by(this auto &&self, E &&executor) {
    self._executor = std::forward<E>(executor);
    return std::forward<decltype(self)>(self);
  }

protected:
  std::optional<std::function<void()>> _next_resume;

  std::shared_ptr<Executor> _executor;
};

template <class ResultType, class Executor>
class Next : public Coro<ResultType> {
private:
  using Base = Coro<ResultType>;

protected:
  using typename Base::Friend;

  using PromiseBase = NextPromiseBase<ResultType, Executor>;

public:
  using result_type = PromiseBase::result_type;
  using executor_type = PromiseBase::executor_type;

  template <class E>
    requires std::constructible_from<std::shared_ptr<Executor>, E>
  auto &&by(this auto &&self, E &&executor) {
    self.get_handle().promise().by(std::forward<E>(executor));
    return std::forward<decltype(self)>(self);
  }

  template <class Self>
  result_type block(this Self &&self) {
    return coro::block(std::forward<Self>(self));
  }

  void detach(this auto &&self) {
    LOG5("task detach");
    coro::detach(std::move(self));
  }
  template <class E>
    requires std::constructible_from<std::shared_ptr<Executor>, E>
  void detach(this auto &&self, E &&executor) {
    self.by(std::forward<E>(executor)).detach();
  }
};
XSL_CORO_NE
#endif
