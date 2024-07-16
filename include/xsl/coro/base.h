#pragma once
#include <type_traits>
#ifndef XSL_CORO_BASE
#  define XSL_CORO_BASE
#  include "xsl/coro/block.h"
#  include "xsl/coro/def.h"
#  include "xsl/coro/detach.h"
#  include "xsl/logctl.h"

#  include <functional>
#  include <memory>
#  include <optional>
XSL_CORO_NB

template <class Promise>
class AwaiterBase {
public:
  using promise_type = Promise;
  using result_type = typename promise_traits<Promise>::result_type;
  using executor_type = typename promise_traits<Promise>::executor_type;

  AwaiterBase(std::coroutine_handle<promise_type> handle) : _handle(handle) {}
  AwaiterBase(AwaiterBase &&another) noexcept : _handle(std::exchange(another._handle, {})) {}
  AwaiterBase &operator=(AwaiterBase &&another) noexcept {
    _handle = std::exchange(another._handle, {});
    return *this;
  }
  AwaiterBase(const AwaiterBase &) = delete;
  AwaiterBase &operator=(const AwaiterBase &) = delete;
  ~AwaiterBase() {
    DEBUG("TaskAwaiter destructor for {}", (uint64_t)_handle.address());
    if (_handle) {
      assert(_handle.done());
      _handle.destroy();
    }
  }

  template <class _Promise>
  void await_suspend(std::coroutine_handle<_Promise> handle) {
    DEBUG("await_suspend: {} -> {}", (uint64_t)_handle.address(), (uint64_t)handle.address());
    if constexpr (!std::is_same_v<typename promise_traits<_Promise>::executor_type,
                                  executor_type>) {
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
    DEBUG("task await_resume for {}", (uint64_t)_handle.address());
    return *_handle.promise();
  }

protected:
  std::coroutine_handle<promise_type> _handle;
};

template <typename ResultType, typename Executor>
class CoroBase {
protected:
  class PromiseBase {
  public:
    using result_type = ResultType;
    using executor_type = Executor;

    PromiseBase() : _result(std::nullopt), _next_resume(std::nullopt), _executor(nullptr) {}

    auto get_return_object(this auto &&self) {
      DEBUG("get_return_object");
      using promise_type = std::remove_cvref_t<decltype(self)>;
      using coro_type = promise_type::coro_type;
      return coro_type{
          std::coroutine_handle<std::remove_cvref_t<decltype(self)>>::from_promise(self)};
    }

    std::suspend_always final_suspend() noexcept {
      DEBUG("final_suspend");
      return {};
    }

    void unhandled_exception() { _result = std::unexpected{std::current_exception()}; }

    result_type operator*() {
      DEBUG("operator*");
      if (*_result) {
        if constexpr (std::is_same_v<result_type, void>) {
          return **_result;
        } else {
          return std::move(**_result);
        }
      }
      std::rethrow_exception(_result->error());
    }

    auto &&by(this auto &&self, const std::shared_ptr<Executor> &executor) {
      self._executor = executor;
      return std::forward<decltype(self)>(self);
    }
    template <class Promise>

    void next(std::coroutine_handle<Promise> handle) {
      _next_resume = [handle]() mutable { handle.promise().resume(handle); };
    }

    template <class Promise>
    void resume(std::coroutine_handle<Promise> handle) {
      this->dispatch([handle, this]() mutable {
        DEBUG("task resume {}", (uint64_t)handle.address());
        handle();
        DEBUG("task resume {} done", (uint64_t)handle.address());
        if (handle.done()) {
          DEBUG("task resume handle done");
          if (this->_next_resume) {
            DEBUG("task resume next_resume");
            (*this->_next_resume)();
          }
        }
      });
    }

    const std::shared_ptr<Executor> &executor() const { return _executor; }

    template <typename F>
      requires std::invocable<F>
    void dispatch(F &&f) {
      if (this->executor()) {
        this->_executor->schedule(std::move(f));
      } else {
        f();
      }
    }

  protected:
    std::optional<Result<result_type>> _result;

    std::optional<std::function<void()>> _next_resume;

    std::shared_ptr<Executor> _executor;
  };

  template <typename Base>
  class Promise : public Base {
    using Base::_result;

  public:
    using result_type = typename Base::result_type;
    void return_value(result_type &&value) {
      DEBUG("return_value");
      _result = Result<result_type>(std::move(value));
    }
  };

  template <typename Base>
    requires std::same_as<typename Base::result_type, void>
  class Promise<Base> : public Base {
    using Base::_result;

  public:
    using result_type = typename Base::result_type;
    void return_void() { _result = Result<void>(); }
  };

public:
  using result_type = PromiseBase::result_type;
  using executor_type = PromiseBase::executor_type;

  CoroBase() = default;

  CoroBase(CoroBase &&task) noexcept = default;

  CoroBase &operator=(CoroBase &&another) noexcept = default;

  CoroBase(const CoroBase &) = delete;

  CoroBase &operator=(const CoroBase &) = delete;

  ~CoroBase() { assert(!_handle); }  // task should be moved to Final/TaskAwaiter

  auto &&by(this auto &&self, std::shared_ptr<executor_type> &executor) {
    self._handle.promise().by(executor);
    return std::forward<decltype(self)>(self);
  }

  void detach(this auto &&self) {
    DEBUG("task detach");
    coro::detach(self._co_await());
  }

  void detach(std::shared_ptr<executor_type> &executor) { this->by(executor).detach(); }

  result_type block(this auto &&self) {
    auto tmp = self._co_await();
    return coro::block(tmp);
  }

  decltype(auto) operator co_await(this auto &&self) {
    DEBUG("move handle to TaskAwaiter");
    return self._co_await();
  }

protected:
  auto _co_await(this auto &&self) -> typename std::remove_cvref_t<decltype(self)>::awaiter_type {
    return std::exchange(self._handle, {});
  }
};
XSL_CORO_NE
#endif
