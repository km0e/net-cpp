#pragma once
#ifndef XSL_CORO_BASE
#  define XSL_CORO_BASE
#  include "xsl/coro/def.h"
#  include "xsl/logctl.h"

#  include <cassert>
#  include <optional>
#  include <type_traits>
XSL_CORO_NB

template <class Promise>
class Awaiter {
public:
  using promise_type = Promise;
  using result_type = typename promise_traits<Promise>::result_type;
  using executor_type = typename promise_traits<Promise>::executor_type;

  Awaiter(std::coroutine_handle<promise_type> handle) : _handle(handle) {}
  Awaiter(Awaiter &&another) noexcept : _handle(std::exchange(another._handle, {})) {}
  Awaiter &operator=(Awaiter &&another) noexcept {
    _handle = std::exchange(another._handle, {});
    return *this;
  }
  Awaiter(const Awaiter &) = delete;
  Awaiter &operator=(const Awaiter &) = delete;
  ~Awaiter() {
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

class HandleGetter {
protected:
  auto &&get_handle(this auto &&self) { return self._handle; }
};

template <typename ResultType>
class Coro : public HandleGetter {
protected:
  using Friend = HandleGetter;

  class PromiseBase {
  public:
    using result_type = ResultType;
    using executor_type = void;

    PromiseBase() : _result(std::nullopt) {}

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

    void unhandled_exception() { this->_result = std::unexpected{std::current_exception()}; }

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

    template <class Promise>
    void resume(std::coroutine_handle<Promise>) {
      assert(false && "coro base cannot resume");
    }

    template <typename F>
      requires std::invocable<F>
    void dispatch(F &&) {
      assert(false && "coro base cannot dispatch");
    }

  protected:
    std::optional<Result<result_type>> _result;
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

  Coro() = default;

  Coro(Coro &&task) noexcept = default;

  Coro &operator=(Coro &&another) noexcept = default;

  Coro(const Coro &) = delete;

  Coro &operator=(const Coro &) = delete;

  decltype(auto) operator co_await(this auto &&self) {
    DEBUG("move handle to TaskAwaiter");
    return self._co_await();
  }

protected:
  auto _co_await(this auto &&self) -> typename std::remove_cvref_t<decltype(self)>::awaiter_type {
    return std::exchange(self.get_handle(), {});
  }
};
XSL_CORO_NE
#endif
