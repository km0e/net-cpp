#pragma once
#include <utility>
#ifndef XSL_CORO_DEF
#  define XSL_CORO_DEF
#  define XSL_CORO_NB namespace xsl::coro {
#  define XSL_CORO_NE }
#  include <coroutine>
#  include <exception>
#  include <expected>
XSL_CORO_NB

/*
void test1() {
  auto executor = std::make_shared<xsl::coro::NoopExecutor>();
  int value = 0;
  auto task = [&]() -> xsl::coro::Task<void> {
    value = 1;
    co_return;
  }();
  task.by(executor).detach();
  assert(value == 1);
}

void test2() {
  auto executor = std::make_shared<xsl::coro::NoopExecutor>();
  int value = 0;
  auto task = [](int& value) -> xsl::coro::Task<void> {
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

struct no_executor {};

template <class Promise>
class promise_traits {
public:
  using result_type = typename Promise::result_type;
  using executor_type = typename Promise::executor_type;
};

struct noop_coroutine {
  struct promise_type {
    using result_type = void;
    using executor_type = no_executor;
    noop_coroutine get_return_object() { return noop_coroutine{}; }
    std::suspend_never initial_suspend() { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    void return_void() {}
    void unhandled_exception() {}
    template <class F>
    void dispatch(F&&) {}
    template <class Promise>
    void resume(std::coroutine_handle<Promise>) {}
  };
  using promise_type = promise_type;
};

template <class Awaiter, class Coroutine = noop_coroutine>
concept Awaitable = requires() { [](Awaiter& a) -> Coroutine { co_await a; }; };

template <class Awaiter>
class awaiter_traits {
public:
  using result_type = typename Awaiter::result_type;
};

template <class ToAwaiter>
using to_awaiter_t = decltype(operator co_await(std::declval<ToAwaiter>()));

template <class ResultType>
using Result = std::expected<ResultType, std::exception_ptr>;

XSL_CORO_NE

#endif  // XSL_CORO_DEF
