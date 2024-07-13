#pragma once
#ifndef XSL_CORO_FINAL
#  define XSL_CORO_FINAL
#  include "xsl/coro/def.h"

#  include <cassert>
#  include <coroutine>
#  include <cstddef>
#  include <exception>
#  include <expected>
#  include <optional>
#  include <type_traits>
#  include <utility>
XSL_CORO_NB

template <class ResultType>
class FinalPromise;

template <class ResultType>
class Final {
  class FinalPromiseBase {
  public:
    using result_type = ResultType;
    using executor_type = no_executor;
    FinalPromiseBase() : _result(std::nullopt) {}
    auto get_return_object(this auto &&self) {
      return Final{std::coroutine_handle<std::remove_cvref_t<decltype(self)>>::from_promise(self)};
    }

    std::suspend_never initial_suspend() { return {}; }

    std::suspend_always final_suspend() noexcept { return {}; }

    void unhandled_exception() { this->_result = std::unexpected{std::current_exception()}; }

    std::nullptr_t executor() const noexcept { return nullptr; }

    template <class Promise>
    void resume(std::coroutine_handle<Promise> handle) noexcept {
      handle();
    }

    result_type operator*() {
      if (*_result) {
        if constexpr (std::is_same_v<result_type, void>) {
          return **_result;
        } else {
          return std::move(**_result);
        }
      } else {
        std::rethrow_exception(_result->error());
      }
    }

  protected:
    std::optional<Result<result_type>> _result;
  };

public:
  using PromiseBase = FinalPromiseBase;

  using promise_type = FinalPromise<ResultType>;

  explicit Final(std::coroutine_handle<promise_type> handle) noexcept : _handle(handle) {}

  Final(Final &&task) noexcept : _handle(std::exchange(task._handle, {})) {}

  Final(const Final &) = delete;

  ResultType operator*() { return *_handle.promise(); }

  ~Final() {
    assert(!_handle || _handle.done());
    if (_handle) {
      _handle.destroy();
    }
  }

private:
  std::coroutine_handle<promise_type> _handle;
};

template <class _ResultType>
class FinalPromise : public Final<_ResultType>::PromiseBase {
  using Base = Final<_ResultType>::PromiseBase;
  using Base::_result;

public:
  using result_type = typename Base::result_type;

  void return_value(result_type &&value) { this->_result = std::move(value); }
};

template <>
class FinalPromise<void> : public Final<void>::PromiseBase {
  using Base = Final<void>::PromiseBase;
  using Base::_result;

public:
  using result_type = typename Base::result_type;

  void return_void() { this->_result = std::expected<void, std::exception_ptr>{}; }
};

XSL_CORO_NE
#endif  // XSL_CORO_FINAL
