/**
 * @file detach.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Detach a coroutine
 * @version 0.11
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_CORO_DETACH
#  define XSL_CORO_DETACH
#  include "xsl/coro/base.h"
#  include "xsl/coro/def.h"
#  include "xsl/logctl.h"

#  include <cassert>
#  include <coroutine>
#  include <type_traits>
#  include <utility>
XSL_CORO_NB
template <class ResultType>
class Detach;

template <class ResultType>
class DetachPromiseBase : public PromiseBase<ResultType> {
public:
  using executor_type = void;
  using coro_type = Detach<ResultType>;
  std::suspend_never initial_suspend() noexcept { return {}; }
  std::suspend_never final_suspend() noexcept { return {}; }
};
template <class ResultType>
class Detach {
public:
  using promise_type = Promise<DetachPromiseBase<ResultType>>;

  explicit Detach(std::coroutine_handle<promise_type> handle) noexcept : _handle(handle) {}

  Detach(Detach &&task) noexcept : _handle(std::exchange(task._handle, {})) {}

private:
  std::coroutine_handle<promise_type> _handle;
};

template <class Awaiter>
  requires Awaitable<Awaiter, Detach<typename awaiter_traits<Awaiter>::result_type>>
           && (!std::is_reference_v<Awaiter>)
void detach(Awaiter &&awaiter) {  // TODO: use co_await to get executor
  [[maybe_unused]] auto final
      = [](Awaiter &&awaiter) -> Detach<typename awaiter_traits<Awaiter>::result_type> {
    LOG6("detach");
    co_await std::move(awaiter);
  }(std::forward<Awaiter>(awaiter));
}

XSL_CORO_NE
#endif
