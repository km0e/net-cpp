/**
 * @file detach.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Detach a coroutine
 * @version 0.1.3
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_CORO_DETACH
#  define XSL_CORO_DETACH
#  include <xsl/coro/core/base.h>
#  include <xsl/coro/core/context.h>
#  include <xsl/coro/core/executor.h>
#  include <xsl/coro/def.h>
#  include <xsl/type_traits.h>
#  include <xsl/wheel.h>

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
  using has_context = std::bool_constant<true>;
  using coro_type = Detach<ResultType>;
  DetachPromiseBase() noexcept : _ctx() {}

  constexpr std::suspend_always initial_suspend() noexcept { return {}; }
  constexpr std::suspend_never final_suspend() noexcept {
    assert(_self.done());
    _self.destroy();
    return {};
  }

  constexpr void by(this auto&& self, auto&& ctx, std::coroutine_handle<> self_handle) noexcept(
      std::is_nothrow_assignable_v<Rc<CoroContext>, decltype(ctx)>) {
    self._ctx = std::forward<decltype(ctx)>(ctx);
    self._self = self_handle;
  }

  constexpr auto ctx(this auto&& self) noexcept -> like_t<decltype(self), Rc<CoroContext>> {
    return self._ctx;
  }

protected:
  Rc<CoroContext> _ctx;
  std::coroutine_handle<> _self = nullptr;
};
template <class ResultType>
class Detach {
public:
  using promise_type = Promise<DetachPromiseBase<ResultType>>;

  constexpr explicit Detach(std::coroutine_handle<promise_type> handle) noexcept
      : _handle(handle) {}

  constexpr Detach(Detach&& ano) noexcept : _handle(std::exchange(ano._handle, {})) {}

  constexpr void operator()(this auto self, Rc<CoroContext>&& ctx) noexcept {
    co_trace("detach");
    self._handle.promise().by(std::forward<decltype(ctx)>(ctx), self._handle);
    self._handle.promise().ctx()->dispatch([handle = self._handle]() mutable {
      co_trace("detach resume {}", (uint64_t)handle.address());
      handle.resume();
    });
  }

private:
  std::coroutine_handle<promise_type> _handle;
};

template <class Awaiter>
  requires Awaitable<Awaiter, Detach<typename Awaiter::result_type>>
           && (!std::is_reference_v<Awaiter>)
constexpr void detach(Awaiter&& awaiter, Rc<CoroContext>&& ctx) {
  auto d = [](Awaiter awaiter) -> Detach<typename awaiter_traits<Awaiter>::result_type> {
    co_trace("detach");
    co_await std::move(awaiter);
  }(std::forward<Awaiter>(awaiter));
  std::move(d)(std::move(ctx));
}

XSL_CORO_NE
#endif
