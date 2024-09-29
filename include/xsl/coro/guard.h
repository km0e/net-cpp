/**
 * @file guard.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Tmp storage for coroutines
 * @version 0.1
 * @date 2024-09-17
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_CORO_GUARD
#  define XSL_CORO_GUARD
#  include "xsl/coro/def.h"

#  include <memory>
#  include <tuple>
#  include <type_traits>

XSL_CORO_NB

template <class Awaiter, class... Args>
class ArgGuardAwaiter;

template <class Awaiter, class... Args>
class ArgGuardAwaiter : public Awaiter {
private:
  using Base = Awaiter;

public:
  using result_type = Base::result_type;
  using executor_type = typename Base::executor_type;

  constexpr ArgGuardAwaiter(Awaiter &&aw, std::unique_ptr<std::tuple<Args...>> &&tmp_tuple)
      : Base(std::forward<Awaiter>(aw)), _tmp(std::move(tmp_tuple)) {}

  using Base::await_ready;
  using Base::await_resume;
  using Base::await_suspend;

protected:
  std::unique_ptr<std::tuple<Args...>> _tmp;
};

template <class... Args>
ArgGuardAwaiter(Args &&...) -> ArgGuardAwaiter<std::decay_t<Args>...>;

template <class AwaiterGen, class... Args>
class ArgGuard {
public:
  using result_type
      = std::invoke_result_t<AwaiterGen, std::add_lvalue_reference_t<Args>...>::result_type;

  template <class _AwaiterGen, class... _Args>
  constexpr ArgGuard(_AwaiterGen &&awaiter_gen, _Args &&...args)
      : _awaiter_gen(std::forward<_AwaiterGen>(awaiter_gen)),
        _tmp_tuple(std::make_unique<std::tuple<Args...>>(std::forward<_Args>(args)...)) {}

  constexpr auto operator co_await() && {
    return ArgGuardAwaiter(std::move(std::apply(_awaiter_gen, *_tmp_tuple)), std::move(_tmp_tuple));
  }

private:
  AwaiterGen _awaiter_gen;
  std::unique_ptr<std::tuple<Args...>> _tmp_tuple;
};

template <class... Args>
ArgGuard(Args &&...) -> ArgGuard<std::decay_t<Args>...>;

XSL_CORO_NE
#endif
