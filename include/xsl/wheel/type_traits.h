#pragma once
#ifndef XSL_WHEEL_TYPE_TRAITS
#  define XSL_WHEEL_TYPE_TRAITS
#  include "xsl/wheel/def.h"

#  include <concepts>
#  include <cstddef>
#  include <type_traits>
#  include <utility>
XSL_WHEEL_NB
namespace type_traits {
  // Template unit
  template <class T1, class T2>
  struct _2 {
    typedef T1 type1;  ///< type1
    typedef T2 type2;  ///< type2
    typedef _2 self;   ///< self type
  };
  // Template unit
  template <class T1, class T2, class T3>
  struct _3 {
    typedef T1 type1;  ///< type1
    typedef T2 type2;  ///< type2
    typedef T3 type3;  ///< type3
    typedef _3 self;   ///< self type
  };
  // Template unit
  template <class... Ts>
  struct _n {
    typedef _n self;///< self type
  };
  namespace impl_size {
    template <class Pack>
    struct size;

    template <template <class...> class Pack, class... Ts>
    struct size<Pack<Ts...>> : std::integral_constant<std::size_t, sizeof...(Ts)> {};
  }  // namespace impl_size

  template <class Pack>
  inline constexpr std::size_t size_v = impl_size::size<Pack>::value;

  namespace impl_find {
    template <template <class L, class R> class Pred, class T, class Pack, std::size_t Index = 0>
      requires requires {
        { Pred<T, Pack>::value } -> std::convertible_to<bool>;
      }
    struct find_if;

    template <template <class L, class R> class Pred, class T, template <class...> class Pack,
              class This, class... Rest, std::size_t Index>
    struct find_if<Pred, T, Pack<This, Rest...>, Index>
        : std::conditional_t<Pred<T, This>::value, std::integral_constant<std::size_t, Index>,
                             find_if<Pred, T, Pack<Rest...>, Index + 1>> {};
    template <template <class L, class R> class Pred, class T, template <class...> class Pack,
              std::size_t Index>
    struct find_if<Pred, T, Pack<>, Index> : std::integral_constant<std::size_t, Index> {};
  }  // namespace impl_find

  template <template <class L, class R> class Pred, class T, class Pack>
  inline constexpr std::size_t find_if_v = impl_find::find_if<Pred, T, Pack>::value;

  template <class T, class Pack>
  inline constexpr bool existing_v = find_if_v<std::is_same, T, Pack> != size_v<Pack>;

  template <class T, class Pack>
  using existing = std::bool_constant<existing_v<T, Pack>>;
  //

  namespace impl_remove {
    template <template <class L, class R> class Pred, class T, class Pack, class... Checked>
      requires requires {
        { Pred<T, Pack>::value } -> std::convertible_to<bool>;
      }
    struct remove_first_if;

    template <template <class L, class R> class Pred, class T, template <class...> class Pack,
              class This, class... Rest, class... Checked>
    struct remove_first_if<Pred, T, Pack<This, Rest...>, Checked...>
        : std::conditional_t<Pred<T, This>::value, std::type_identity<Pack<Checked..., Rest...>>,
                             remove_first_if<Pred, T, Pack<Rest...>, Checked..., This>> {};

    template <template <class L, class R> class Pred, class T, template <class...> class Pack,
              class... Checked>
    struct remove_first_if<Pred, T, Pack<>, Checked...> : std::type_identity<Pack<Checked...>> {};

    template <template <class L, class R> class Pred, class Opts, class Pack, class... Checked>
    struct remove_first_of_if;

    template <template <class L, class R> class Pred, class Opts, template <class...> class Pack,
              class This, class... Rest, class... Checked>
    struct remove_first_of_if<Pred, Opts, Pack<This, Rest...>, Checked...>
        : std::conditional_t<find_if_v<Pred, This, Opts> != size_v<Opts>,
                             _2<Pack<Checked..., Rest...>, This>,
                             remove_first_of_if<Pred, Opts, Pack<Rest...>, Checked..., This>> {};

    template <template <class L, class R> class Pred, class Opts, template <class...> class Pack,
              class... Checked>
    struct remove_first_of_if<Pred, Opts, Pack<>, Checked...> : _2<Pack<Checked...>, void> {};
  }  // namespace impl_remove

  template <template <class L, class R> class Pred, class T, class Pack>
  using remove_first_if_t = typename impl_remove::remove_first_if<Pred, T, Pack>::type;

  template <template <class L, class R> class Pred, class Opts, class Pack>
  using remove_first_of_if = typename impl_remove::remove_first_of_if<Pred, Opts, Pack>::self;

  namespace impl_swap {
    template <class LeftPack, class RightPack>
    struct swap;
    //
    template <template <class...> class Left, class... Ts1, template <class...> class Right,
              class... Ts2>
    struct swap<Left<Ts1...>, Right<Ts2...>> : _2<Left<Ts2...>, Right<Ts1...>> {};
  }  // namespace impl_swap
  //
  template <class LeftPack, class RightPack>
  using swap_t = typename impl_swap::swap<LeftPack, RightPack>::self;

  namespace impl_copy {
    template <class From, class To>
    struct copy;

    template <template <class...> class From, template <class...> class To, class... Ts,
              class... Us>
    struct copy<From<Ts...>, To<Us...>> : public std::type_identity<To<Ts...>> {};

  }  // namespace impl_copy
  template <class From, class To>
  using copy_t = typename impl_copy::copy<From, To>::type;

  namespace impl_verify {
    template <class LMBD, class Pack, class V = void>
    inline constexpr bool verify_v = false;
    template <class LMBD, class... T>
    inline constexpr bool
        verify_v<LMBD, _n<T...>, std::void_t<decltype(std::declval<LMBD>()(std::declval<T>()...))>>
        = true;
  }  // namespace impl_verify
  /**
   @brief Verify if the lambda can be applied to the types

   @tparam LMBD
   @tparam T
   */
  template <class LMBD, class... T>
  inline constexpr bool verify_v = impl_verify::verify_v<LMBD, _n<T...>>;

  // template <class Base, class Derive>
  // inline constexpr bool base
  //     = verify_v<decltype([]<class T, class U>(T, U)
  //                             -> decltype(eval_type<T *&>() = eval_type<U *>(), 0) { return 0;
  //                             }),
  //                _n<Base, Derive>>;

  namespace impl_is_same_pack {
    template <class Pack1, class Pack2>
    struct is_same_pack : std::false_type {};

    template <template <class...> class Pack, class... Ts1, class... Ts2>
    struct is_same_pack<Pack<Ts1...>, Pack<Ts2...>> : std::true_type {};
  }  // namespace impl_is_same_pack

  template <class Pack1, class Pack2>
  struct is_same_pack : impl_is_same_pack::is_same_pack<Pack1, Pack2> {};

  template <class Pack1, class Pack2>
  inline constexpr bool is_same_pack_v = impl_is_same_pack::is_same_pack<Pack1, Pack2>::value;

  namespace impl_inner {
    template <class T>
    struct inner;

    template <template <class...> class Pack, class T, class... Ts>
    struct inner<Pack<T, Ts...>> : std::type_identity<T> {};
  }  // namespace impl_inner

  template <class T>
  using inner_t = typename impl_inner::inner<T>::type;
}  // namespace type_traits

XSL_WHEEL_NE
#endif
