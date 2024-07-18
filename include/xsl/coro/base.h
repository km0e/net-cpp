#pragma once
#ifndef XSL_CORO_BASE
#  define XSL_CORO_BASE
#  include "xsl/coro/chain.h"
#  include "xsl/coro/def.h"
#  include "xsl/logctl.h"

#  include <cassert>
#  include <optional>
#  include <type_traits>
#  include <utility>
XSL_CORO_NB
template <class ResultType>
class PromiseBase {
public:
  using result_type = ResultType;
  using executor_type = void;

  PromiseBase() : _result(std::nullopt) {}

  auto get_return_object(this auto &&self) noexcept {
    DEBUG("get_return_object");
    using promise_type = std::decay_t<decltype(self)>;
    using coro_type = promise_type::coro_type;
    return coro_type{std::coroutine_handle<promise_type>::from_promise(self)};
  }

  std::suspend_never initial_suspend() const noexcept { return {}; }

  std::suspend_always final_suspend() const noexcept {
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
  void resume(std::coroutine_handle<Promise> handle) noexcept(noexcept(handle.resume())) {
    handle();
  }

  template <std::invocable F>
  void dispatch(F &&f) noexcept(noexcept(std::forward<F>(f)())) {
    f();
  }

protected:
  std::optional<Result<result_type>> _result;
};

template <class Base>
class Promise : public Base {
protected:
  using Base::_result;

public:
  using typename Base::result_type;
  void return_value(result_type &&value) {
    DEBUG("return_value");
    _result = Result<result_type>(std::move(value));
  }
};

template <class Base>
  requires std::same_as<typename Base::result_type, void>
class Promise<Base> : public Base {
protected:
  using Base::_result;

public:
  using typename Base::result_type;
  void return_void() { _result = Result<void>(); }
};

class HandleControl {
protected:
  auto &&get_handle(this auto &&self) { return self._handle; }
  auto move_handle(this auto &&self) { return std::exchange(self._handle, {}); }
};

template <class ResultType>
class Coro : public HandleControl {
protected:
  using Friend = HandleControl;

public:
  using result_type = PromiseBase<ResultType>::result_type;
  using executor_type = PromiseBase<ResultType>::executor_type;

  auto operator co_await(this auto &&self) -> typename std::decay_t<decltype(self)>::awaiter_type {
    DEBUG("move handle to TaskAwaiter");
    return self.move_handle();
  }

  auto transform(this auto &&self, std::invocable<result_type> auto &&f) {
    using awaiter_type = typename std::decay_t<decltype(self)>::awaiter_type;
    return ChainAwaiter<awaiter_type>(self.move_handle()).transform(std::forward<decltype(f)>(f));
  }
};
XSL_CORO_NE
#endif
