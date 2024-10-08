/**
 * @file detach.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Detach a coroutine
 * @version 0.12
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
#  include "xsl/coro/executor.h"
#  include "xsl/logctl.h"

#  include <cassert>
#  include <concepts>
#  include <coroutine>
#  include <cstddef>
#  include <memory>
#  include <type_traits>
#  include <utility>
XSL_CORO_NB
template <class ResultType>
class Detach;

template <class ResultType>
class DetachPromiseBase : public PromiseBase<ResultType> {
protected:
  using PromiseBase<ResultType>::_result;

public:
  using executor_type = ExecutorBase;
  using coro_type = Detach<ResultType>;
  DetachPromiseBase() noexcept : _executor(nullptr) {}

  constexpr std::suspend_always initial_suspend() noexcept { return {}; }
  constexpr std::suspend_never final_suspend() noexcept { return {}; }

  constexpr const std::shared_ptr<ExecutorBase> &executor() const noexcept { return _executor; }

  constexpr void by(this auto &&self, auto &&executor) {
    self._executor = std::forward<decltype(executor)>(executor);
  }

private:
  std::shared_ptr<ExecutorBase> _executor;
};
template <class ResultType>
class Detach {
public:
  using promise_type = Promise<DetachPromiseBase<ResultType>>;

  constexpr explicit Detach(std::coroutine_handle<promise_type> handle) noexcept
      : _handle(handle) {}

  constexpr Detach(Detach &&ano) noexcept : _handle(std::exchange(ano._handle, {})) {}

  constexpr void operator()(this auto self,
                            std::convertible_to<std::shared_ptr<ExecutorBase>> auto &&executor) {
    LOG6("detach");
    self._handle.promise().by(std::forward<decltype(executor)>(executor));
    self._handle.resume();
  }

private:
  std::coroutine_handle<promise_type> _handle;
};

template <class Awaiter,
          std::convertible_to<std::shared_ptr<ExecutorBase>> Executor = std::nullptr_t>
  requires Awaitable<Awaiter, Detach<typename Awaiter::result_type>>
           && (!std::is_reference_v<Awaiter>)
constexpr void detach(Awaiter &&awaiter, Executor &&executor = nullptr) {
  auto d = [](Awaiter &&awaiter) -> Detach<typename awaiter_traits<Awaiter>::result_type> {
    LOG6("detach");
    co_await std::move(awaiter);
  }(std::forward<Awaiter>(awaiter));
  std::move(d)(std::forward<decltype(executor)>(executor));
}

XSL_CORO_NE
#endif
