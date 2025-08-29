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
template <std::size_t N>
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
template <template <class T> class T = _n>
struct Wrapper {};
template <class... Ts>
struct BaseOn {};

// using for resolver

namespace impl {
  struct Rest;
  template <class Pred, class... Flags>
  struct Item {};

  /**
  @brief format a single flag

  @tparam Flag
   */
  template <class Flag>
  struct off_fmt_single : std::type_identity<Item<std::is_same<Placeholder, void>, Flag>> {};
  /**
  @brief format a set of flags

  @tparam Flags
   */
  template <class... Flags>
  struct off_fmt_single<set<Flags...>>
      : std::type_identity<Item<std::is_same<Placeholder, void>, Flags...>> {};
  /**
  @brief forward the flag if it is already formatted

  @tparam Pred
  @tparam Flags
   */
  template <class Default, class T, template <class, class> class Pred, class Flag, class... Flags>
  struct off_fmt_single<Item<Pred<Default, T>, Flag, Flags...>>
      : std::type_identity<Item<Pred<Default, T>, Flag, Flags...>> {};
  /**
    @brief forward the flag if it is already formatted

    @tparam Pred
    @tparam Flags
     */
  template <class Default, class T, template <class, class> class Pred>
  struct off_fmt_single<Item<Pred<Default, T>>>
      : std::type_identity<Item<Pred<Default, T>, Default>> {};
  /**
    @brief forward the flag if it is already formatted

    @tparam Pred
    @tparam Flags
     */
  template <>
  struct off_fmt_single<Rest> : std::type_identity<Rest> {};

  template <class FullFlag>
  struct off_fmt;

  template <template <class...> class FlagPack, class... FlagItems>
  struct off_fmt<FlagPack<FlagItems...>>
      : std::type_identity<_n<typename off_fmt_single<FlagItems>::type...>> {};

  static_assert(
      std::is_same_v<off_fmt<_n<int>>::type, _n<Item<std::is_same<Placeholder, void>, int>>>);

  static_assert(std::is_same_v<off_fmt<_n<int, set<int, float>>>::type,
                               _n<Item<std::is_same<Placeholder, void>, int>,
                                  Item<std::is_same<Placeholder, void>, int, float>>>);

  static_assert(std::is_same_v<off_fmt<_n<Item<always_true<Placeholder, void>>>>::type,
                               _n<Item<always_true<Placeholder, void>, Placeholder>>>);

  template <class FlagSet, class FullFeatureFlagItemSet, class... CompleteFlags>
  struct off_fill;

  template <class Pair, class FullFeatureFlagItemSet, class... CompleteFlags>
  struct off_fill_helper;

  template <class Default, class Pair, class... FlagItems, class... CompleteFlags>
  struct off_fill_helper<Default, Pair, _n<FlagItems...>, CompleteFlags...>
      : std::type_identity<
            off_fill<typename Pair::type1, _n<FlagItems...>, CompleteFlags...,
                     std::conditional_t<std::is_same_v<typename Pair::type2, void>,  /// Not Found
                                        Default, typename Pair::type2>>> {};

  template <class... Flags, template <class T, class U> class Pred, class Default,
            class... FlagOpts, class... FlagItems, class... CompleteFlags, class Reserved>
  struct off_fill<_n<Flags...>, _n<Item<Pred<Default, Reserved>, FlagOpts...>, FlagItems...>,
                  CompleteFlags...>
      : off_fill_helper<Default,
                        typename remove_first_of_if<Pred, _n<FlagOpts...>, _n<Flags...>>::self,
                        _n<FlagItems...>, CompleteFlags...>::type {};
  template <class... Flags, class... FlagItems, class... CompleteFlags>
  struct off_fill<_n<Flags...>, _n<Rest, FlagItems...>, CompleteFlags...>
      : std::type_identity<_n<CompleteFlags..., Flags...>> {};

  template <class... Flags, class... CompleteFlags>
  struct off_fill<_n<Flags...>, _n<>, CompleteFlags...> : std::type_identity<_n<CompleteFlags...>> {
  };

  static_assert(std::is_same_v<off_fill<_n<>, _n<>>::type, _n<>>);

  static_assert(std::is_same_v<off_fill<_n<int>, _n<>>::type, _n<>>);

  static_assert(std::is_same_v<
                off_fill<_n<int>, _n<Item<std::is_same<Placeholder, void>, int>>>::type, _n<int>>);
  static_assert(
      std::is_same_v<off_fill<_n<int>, _n<Item<std::is_same<Placeholder, void>, float>>>::type,
                     _n<Placeholder>>);
  static_assert(std::is_same_v<
                off_fill<_n<int>, _n<Item<always_true<Placeholder, void>, void>>>::type, _n<int>>);
  static_assert(
      std::is_same_v<off_fill<_n<int, char>, _n<Item<always_true<Placeholder, void>, void>,
                                                Item<always_true<Placeholder, void>, void>>>::type,
                     _n<int, char>>);
  static_assert(std::is_same_v<
                off_fill<_n<Wrapper<BaseOn>>, _n<Item<is_same_pack<Placeholder>, Wrapper<>>>>::type,
                _n<Wrapper<BaseOn>>>);
  template <class FullFlag, class... Flags>
  using off_compose_t
      = copy_t<typename off_fill<_n<Flags...>, typename off_fmt<FullFlag>::type>::type, FullFlag>;

}  // namespace impl
using impl::Item;
template <class FullFlag, class... Flags>
using select_feature_flags_t = impl::off_compose_t<FullFlag, Flags...>;

static_assert(std::is_same_v<select_feature_flags_t<set<int, float>, int, float>, set<int, float>>);
static_assert(std::is_same_v<select_feature_flags_t<set<int, float>, float, int>, set<int, float>>);
static_assert(
    std::is_same_v<select_feature_flags_t<set<int, float>, int, double>, set<int, Placeholder>>);
static_assert(
    std::is_same_v<select_feature_flags_t<set<int, float>, double, int>, set<int, Placeholder>>);
static_assert(std::is_same_v<select_feature_flags_t<set<int, float>, float, double>,
                             set<Placeholder, float>>);
static_assert(std::is_same_v<select_feature_flags_t<set<int, float>, double, float>,
                             set<Placeholder, float>>);
static_assert(std::is_same_v<select_feature_flags_t<set<int, float>, double, char>,
                             set<Placeholder, Placeholder>>);

static_assert(
    std::is_same_v<select_feature_flags_t<set<int, float>, double, int, float>, set<int, float>>);

static_assert(
    std::is_same_v<select_feature_flags_t<set<set<int, float>, char>, int, char>, set<int, char>>);
static_assert(std::is_same_v<select_feature_flags_t<set<set<int, float>, char>, float, char>,
                             set<float, char>>);
static_assert(
    std::is_same_v<select_feature_flags_t<set<set<int, float>, char>, char, int>, set<int, char>>);
static_assert(std::is_same_v<select_feature_flags_t<set<set<char, float>, char>, char, int>,
                             set<char, Placeholder>>);
static_assert(std::is_same_v<select_feature_flags_t<set<set<char, float>, char>, char, char>,
                             set<char, char>>);
static_assert(
    std::is_same_v<select_feature_flags_t<set<set<char, float>, set<char, double>>, char, double>,
                   set<char, double>>);
static_assert(
    std::is_same_v<select_feature_flags_t<set<set<char, float>, set<char, double>>, char, char>,
                   set<char, char>>);

static_assert(std::is_same_v<select_feature_flags_t<set<Item<is_same_pack<Placeholder>, Wrapper<>>>,
                                                    Wrapper<BaseOn>>,
                             set<Wrapper<BaseOn>>>);

namespace impl {

  template <class R, template <class T, class U> class Pred, class... Opts>
  struct merge;

  template <class R, template <class T, class U> class Pred, class Opt, class... Opts>
  struct merge<R, Pred, Opt, Opts...> : merge<typename Pred<R, Opt>::type, Pred, Opts...> {};

  template <class R, template <class T, class U> class Pred>
  struct merge<R, Pred> : std::type_identity<R> {};

}  // namespace impl

using impl::Item;
using impl::Rest;

template <template <class T, class U> class Pred, class Flag, class... Flags>
using merge_feature_flags_t = typename impl::merge<Flag, Pred, Flags...>::type;

XSL_NE
#endif
