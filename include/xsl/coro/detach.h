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
  using coro_type = Detach<ResultType>;
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
void detach(Awaiter &&awaiter) {//TODO: use co_await to get executor
  [[maybe_unused]] auto final
      = [](Awaiter &&awaiter) -> Detach<typename awaiter_traits<Awaiter>::result_type> {
    LOG6("detach");
    if constexpr (std::is_reference_v<Awaiter>) {
      co_await awaiter;
    } else {
      Awaiter awaiter_copy = std::forward<Awaiter>(awaiter);
      co_await awaiter_copy;
    }
  }(std::forward<Awaiter>(awaiter));
}

XSL_CORO_NE
#endif
