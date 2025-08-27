/**
 * @file feature.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Feature flags
 * @version 0.1.3
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_FEATURE
#  define XSL_FEATURE
#  include <xsl/def.h>
#  include <xsl/type_traits.h>

#  include <type_traits>

XSL_NB
struct Placeholder {};

template <class... Opts>
using set = _n<Opts...>;

template <template <class T, class U> class Pred, class... Opts>
struct merge {};

struct node {};

struct Tcp {};
struct Udp {};
struct Ip {};

struct TcpIp : Tcp, Ip {};
struct UdpIp : Udp, Ip {};
struct Ipv4 : Ip {};
struct Ipv6 : Ip {};
struct TcpIpv4 : TcpIp {};
struct TcpIpv6 : TcpIp {};
struct UdpIpv4 : UdpIp {};
struct UdpIpv6 : UdpIp {};

struct Static {};
struct Exact {};
struct Shared {};
struct Raw {};
template <class T>
struct In {};
template <class T>
struct Out {};
template <class T>
struct InOut {};
struct Own {};
struct Dyn {};
struct Unsafe {};
struct Lazy {};
struct Blocking {};
// using for resolver

namespace impl {
  template <template <class T, class U> class Pred, class... Flags>
    requires requires {
      { Pred<int, int>::value } -> std::convertible_to<bool>;
    }
  struct Item {};
  /**
  @brief format a single flag

  @tparam Flag
   */
  template <class Flag>
  struct off_fmt_single : std::type_identity<Item<std::is_same, Flag>> {};
  /**
  @brief format a set of flags

  @tparam Flags
   */
  template <class... Flags>
  struct off_fmt_single<set<Flags...>> : std::type_identity<Item<std::is_same, Flags...>> {};
  /**
  @brief forward the flag if it is already formatted

  @tparam Pred
  @tparam Flags
   */
  template <template <class L, class R> class Pred, class... Flags>
  struct off_fmt_single<Item<Pred, Flags...>> : std::type_identity<Item<Pred, Flags...>> {};

  template <class FullFlag>
  struct off_fmt;

  template <template <class...> class FlagPack, class... FlagItems>
  struct off_fmt<FlagPack<FlagItems...>>
      : std::type_identity<_n<typename off_fmt_single<FlagItems>::type...>> {};

  static_assert(std::is_same_v<off_fmt<_n<int>>::type, _n<Item<std::is_same, int>>>);

  static_assert(std::is_same_v<off_fmt<_n<int, set<int, float>>>::type,
                               _n<Item<std::is_same, int>, Item<std::is_same, int, float>>>);

  static_assert(std::is_same_v<off_fmt<_n<Item<always_true>>>::type, _n<Item<always_true>>>);

  template <class FlagSet, class FullFeatureFlagItemSet, class... CompleteFlags>
  struct off_fill;

  template <class Pair, class FullFeatureFlagItemSet, class... CompleteFlags>
  struct off_fill_helper;

  template <class Pair, class... FlagItems, class... CompleteFlags>
  struct off_fill_helper<Pair, _n<FlagItems...>, CompleteFlags...>
      : std::type_identity<off_fill<typename Pair::type1, _n<FlagItems...>, CompleteFlags...,
                                    std::conditional_t<std::is_same_v<typename Pair::type2, void>,
                                                       Placeholder, typename Pair::type2>>> {};

  template <class... Flags, template <class T, class U> class Pred, class... FlagOpts,
            class... FlagItems, class... CompleteFlags>
  struct off_fill<_n<Flags...>, _n<Item<Pred, FlagOpts...>, FlagItems...>, CompleteFlags...>
      : off_fill_helper<typename remove_first_of_if<Pred, _n<FlagOpts...>, _n<Flags...>>::self,
                        _n<FlagItems...>, CompleteFlags...>::type {};

  template <class... Flags, class... CompleteFlags>
  struct off_fill<_n<Flags...>, _n<>, CompleteFlags...> : std::type_identity<_n<CompleteFlags...>> {
  };

  static_assert(std::is_same_v<off_fill<_n<>, _n<>>::type, _n<>>);

  static_assert(std::is_same_v<off_fill<_n<int>, _n<>>::type, _n<>>);

  static_assert(std::is_same_v<off_fill<_n<int>, _n<Item<std::is_same, int>>>::type, _n<int>>);
  static_assert(
      std::is_same_v<off_fill<_n<int>, _n<Item<std::is_same, float>>>::type, _n<Placeholder>>);
  static_assert(std::is_same_v<off_fill<_n<int>, _n<Item<always_true, void>>>::type, _n<int>>);
  static_assert(std::is_same_v<
                off_fill<_n<int, char>, _n<Item<always_true, void>, Item<always_true, void>>>::type,
                _n<int, char>>);

  template <class FullFlag, class... Flags>
  using off_compose_t
      = copy_t<typename off_fill<_n<Flags...>, typename off_fmt<FullFlag>::type>::type, FullFlag>;
}  // namespace impl
using impl::Item;
template <class FullFlag, class... Flags>
using select_feature_flags_t = impl::off_compose_t<FullFlag, Flags...>;

namespace impl {

  template <class R, template <class T, class U> class Pred, class... Opts>
  struct merge;

  template <class R, template <class T, class U> class Pred, class Opt, class... Opts>
  struct merge<R, Pred, Opt, Opts...> : merge<typename Pred<R, Opt>::type, Pred, Opts...> {};

  template <class R, template <class T, class U> class Pred>
  struct merge<R, Pred> : std::type_identity<R> {};

}  // namespace impl

using impl::Item;

template <template <class T, class U> class Pred, class Flag, class... Flags>
using merge_feature_flags_t = typename impl::merge<Flag, Pred, Flags...>::type;

XSL_NE
#endif
