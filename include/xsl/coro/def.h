/**
 * @file def.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Definition of coroutines
 * @version 0.11
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_CORO_DEF
#  define XSL_CORO_DEF
#  define XSL_CORO_NB namespace xsl::_coro {
#  define XSL_CORO_NE }
#  include <coroutine>
#  include <exception>
#  include <expected>
#  include <utility>
XSL_CORO_NB

/*
void test1() {
  auto executor = std::make_shared<xsl::coro::NoopExecutor>();
  int value = 0;
  auto task = [&]() -> xsl::Task<void> {
    value = 1;
    co_return;
  }();
  task.by(executor).detach();
  assert(value == 1);
}

void test2() {
  auto executor = std::make_shared<xsl::coro::NoopExecutor>();
  int value = 0;
  auto task = [](int& value) -> xsl::Task<void> {
    value = 1;
    co_return;
  }(value);
  task.by(executor).detach();
  assert(value == 1);
}

test1 will throw error: segmentation fault
do not know why, but test2 is ok
so I think the problem is the lambda capture, do not use lambda capture
*/

struct noop_coroutine {
  struct promise_type {
    using result_type = void;
    using executor_type = void;
    noop_coroutine get_return_object() { return noop_coroutine{}; }
    std::suspend_never initial_suspend() { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    void return_void() {}
    void unhandled_exception() {}
    template <class F>
    void dispatch(F &&) {}
    template <class Promise>
    void resume(std::coroutine_handle<Promise>) {}
  };
  using promise_type = promise_type;
};

template <class Awaiter, class Coroutine = noop_coroutine>
concept Awaitable = requires() { [](Awaiter a) -> Coroutine { co_await std::move(a); }; };

template <class Awaiter>
class awaiter_traits {
public:
  using result_type = typename Awaiter::result_type;
};

template <class ToAwaiter>
using to_awaiter_t = decltype(operator co_await(std::declval<ToAwaiter>()));

template <class ResultType>
using Result = std::expected<ResultType, std::exception_ptr>;

class HandleControl {
protected:
  auto &&get_handle(this auto &&self) { return self._handle; }
  auto move_handle(this auto &&self) { return std::exchange(self._handle, {}); }
};

XSL_CORO_NE

#endif  // XSL_CORO_DEF
