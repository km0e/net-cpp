#pragma once
#ifndef XSL_CORO_DEF
#  define XSL_CORO_DEF
#  define XSL_CORO_NAMESPACE_BEGIN namespace xsl::coro {
#  define XSL_CORO_NAMESPACE_END }
#  include <coroutine>
#  include <exception>
#  include <expected>
XSL_CORO_NAMESPACE_BEGIN

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

template <class Awaiter>
class awaiter_traits {
public:
  using result_type = decltype(std::declval<Awaiter>().await_resume());
};

template <class Awaiter>
concept Awaitable = requires(Awaiter awaiter) {
  { awaiter.await_ready() } -> std::same_as<bool>;
  { awaiter.await_suspend(std::coroutine_handle<>{}) };
  { awaiter.await_resume() };
};

template <class T>
concept ToAwaiter = requires(T t) {
  { t.operator co_await() } -> Awaitable;
};

template <class T>
using to_awaiter_t = decltype(std::declval<T>().operator co_await());

template <class ResultType>
using Result = std::expected<ResultType, std::exception_ptr>;

XSL_CORO_NAMESPACE_END

#endif  // XSL_CORO_DEF
