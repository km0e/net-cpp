#pragma once
#ifndef XSL_FEATURE
#  define XSL_FEATURE
#  include "xsl/def.h"
#  include "xsl/wheel/type_traits.h"

#  include <cstdint>
#  include <type_traits>

XSL_NB
namespace feature {
  struct placeholder {};

  template <class... Opts>
  using set = wheel::type_traits::_n<Opts...>;

  struct node {};
  // using for tcp component
  template <class LowerLayer = placeholder>
  struct Tcp {};
  // using for resolver
  template <class LowerLayer = placeholder>
  struct Udp {};
  // using for resolver
  template <uint8_t version = 4>
  struct Ip {};
  struct Exact {};
  struct Raw {};
  template <class T>
  struct In {};
  template <class T>
  struct Out {};
  template <class T>
  struct InOut {};
  struct Own {};
  struct Dyn {};
  // using for resolver

  namespace impl {
    template <class... Tps>
    using type_list = wheel::type_traits::_n<Tps...>;

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
    struct off_fmt_single : wheel::type_traits::_1<Item<std::is_same, Flag>> {};
    /**
    @brief format a set of flags

    @tparam Flags
     */
    template <class... Flags>
    struct off_fmt_single<set<Flags...>> : wheel::type_traits::_1<Item<std::is_same, Flags...>> {};
    /**
    @brief forward the flag if it is already formatted

    @tparam Pred
    @tparam Flags
     */
    template <template <class L, class R> class Pred, class... Flags>
    struct off_fmt_single<Item<Pred, Flags...>> : wheel::type_traits::_1<Item<Pred, Flags...>> {};

    template <class FullFlag>
    struct off_fmt;

    template <template <class...> class FlagPack, class... FlagItems>
    struct off_fmt<FlagPack<FlagItems...>>
        : wheel::type_traits::_1<type_list<typename off_fmt_single<FlagItems>::type...>> {};

    static_assert(
        std::is_same_v<off_fmt<type_list<int>>::type, type_list<Item<std::is_same, int>>>);

    static_assert(
        std::is_same_v<off_fmt<type_list<int, set<int, float>>>::type,
                       type_list<Item<std::is_same, int>, Item<std::is_same, int, float>>>);

    template <class FlagSet, class FullFeatureFlagItemSet, class... CompleteFlags>
    struct off_fill;

    template <class Pair, class FullFeatureFlagItemSet, class... CompleteFlags>
    struct off_fill_helper;

    template <class Pair, class... FlagItems, class... CompleteFlags>
    struct off_fill_helper<Pair, type_list<FlagItems...>, CompleteFlags...>
        : wheel::type_traits::_1<
              off_fill<typename Pair::type1, type_list<FlagItems...>, CompleteFlags...,
                       std::conditional_t<std::is_same_v<typename Pair::type2, void>, placeholder,
                                          typename Pair::type2>>> {};

    template <class... Flags, template <class T, class U> class Pred, class... FlagOpts,
              class... FlagItems, class... CompleteFlags>
    struct off_fill<type_list<Flags...>, type_list<Item<Pred, FlagOpts...>, FlagItems...>,
                    CompleteFlags...>
        : off_fill_helper<typename wheel::type_traits::remove_first_of_if<
                              Pred, type_list<FlagOpts...>, type_list<Flags...>>::self,
                          type_list<FlagItems...>, CompleteFlags...>::type {};

    template <class... Flags, class... CompleteFlags>
    struct off_fill<type_list<Flags...>, type_list<>, CompleteFlags...>
        : wheel::type_traits::_1<type_list<CompleteFlags...>> {};

    static_assert(std::is_same_v<off_fill<type_list<>, type_list<>>::type, type_list<>>);

    static_assert(std::is_same_v<off_fill<type_list<int>, type_list<>>::type, type_list<>>);

    static_assert(std::is_same_v<off_fill<type_list<int>, type_list<Item<std::is_same, int>>>::type,
                                 type_list<int>>);
    static_assert(
        std::is_same_v<off_fill<type_list<int>, type_list<Item<std::is_same, float>>>::type,
                       type_list<placeholder>>);

    template <class FullFlag, class... Flags>
    using off_compose_t = wheel::type_traits::copy_t<
        typename off_fill<type_list<Flags...>, typename off_fmt<FullFlag>::type>::type, FullFlag>;
  }  // namespace impl
  using impl::Item;
  template <class FullFlag, class... Flags>
  using organize_feature_flags_t = impl::off_compose_t<FullFlag, Flags...>;

}  // namespace feature
XSL_NE
#endif
