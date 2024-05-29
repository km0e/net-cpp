#pragma once
#ifndef _XSL_WHEEL_TYPE_TRAITS_H_
#  define _XSL_WHEEL_TYPE_TRAITS_H_
#  include "xsl/wheel/def.h"

#  include <type_traits>
WHEEL_NAMESPACE_BEGIN
namespace type_traits {
  template <class... Ts>
  using _0 = void;
  // Template unit
  template <class T>
  struct _1 {
    typedef T type;
    typedef _1 self;
  };
  // Template unit
  template <class T1, class T2>
  struct _2 {
    typedef T1 type1;
    typedef T2 type2;
    typedef _2 self;
  };
  // Template unit
  template <class T1, class T2, class T3>
  struct _3 {
    typedef T1 type1;
    typedef T2 type2;
    typedef T3 type3;
    typedef _3 self;
  };
  // Template unit
  template <class... Ts>
  struct _n {
    typedef _n self;
  };
  template <class T, class Rep>
  inline constexpr bool existing_v = false;
  //
  template <class T, template <class...> class Rep, class This, class... Rest>
  inline constexpr bool existing_v<T, Rep<This, Rest...>>
      = std::is_same_v<T, This> || existing_v<T, Rep<Rest...>>;

  template <class T, class Rep>
  using existing = std::bool_constant<existing_v<T, Rep>>;
  //

  namespace impl {
    template <class LeftRep, class RightRep>
    struct swap;
    //
    template <template <class...> class Left, class... Ts1, template <class...> class Right,
              class... Ts2>
    struct swap<Left<Ts1...>, Right<Ts2...>> : _2<Left<Ts2...>, Right<Ts1...>> {};
  }  // namespace impl
  //
  template <class LeftRep, class RightRep>
  using swap_t = typename impl::swap<LeftRep, RightRep>::self;
}  // namespace type_traits

WHEEL_NAMESPACE_END
#endif
