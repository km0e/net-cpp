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
#  include "xsl/type_traits.h"

#  include <tuple>
#  include <type_traits>

XSL_CORO_NB

template <class AwaiterType, class TmpStorage>
class GuardAwaiter;

template <class AwaiterType, class TmpStorage>
class GuardAwaiter : public AwaiterType {
private:
  using Base = AwaiterType;

public:
  using result_type = Base::result_type;
  using executor_type = typename Base::executor_type;

  template <class _Awaiter, class... _Args>
  constexpr GuardAwaiter(_Awaiter &&aw, _Args &&...args)
      : Base(std::forward<_Awaiter>(aw)), _tmp_storage(std::forward<_Args>(args)...) {}

  using Base::await_ready;
  using Base::await_resume;
  using Base::await_suspend;

protected:
  TmpStorage _tmp_storage;
};

template <class TmpTuple>
class GuardCreator {
public:
  using result_type = std::decay_t<std::tuple_element_t<0, TmpTuple>>::result_type;

  constexpr GuardCreator(TmpTuple &&tmp_tuple) : _tmp_tuple(std::forward<TmpTuple>(tmp_tuple)) {}

  constexpr auto operator co_await() && {
    using type_pair = remove_first_if<for_each_t<std::decay, TmpTuple>>;
    using awaiter_type = typename type_pair::type1;
    using tmp_storage = typename type_pair::type2;
    return std::make_from_tuple<GuardAwaiter<awaiter_type, tmp_storage>>(_tmp_tuple);
  }

private:
  TmpTuple _tmp_tuple;
};

XSL_CORO_NE
#endif
