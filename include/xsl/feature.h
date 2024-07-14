#pragma once
#include <type_traits>
#ifndef XSL_FEATURE
#  define XSL_FEATURE
#  include "xsl/def.h"
#  include "xsl/wheel/type_traits.h"

#  include <cstdint>

XSL_NB
namespace feature {
  struct placeholder {};

  template <class... Opts>
  using set = wheel::type_traits::_n<Opts...>;

  struct node {};
  // using for tcp component
  struct Tcp {};
  // using for resolver
  struct Udp {};
  // using for resolver
  template <uint8_t version>
  struct Ip {};

  namespace impl {
    template <class... Args>
    using type_list = wheel::type_traits::_n<Args...>;
    template <class FeatureFlagSet, class FullFeatureFlagSet, class CompleteFeatureFlagSet>
    struct origanize_feature_flags;

    template <class... Flags, class... CompleteFlags>
    struct origanize_feature_flags<type_list<Flags...>, type_list<>, type_list<CompleteFlags...>>
        : wheel::type_traits::_1<type_list<CompleteFlags...>> {};

    template <class FlagSet, class... FlagSets, class... CompleteFlags>
    struct origanize_feature_flags<type_list<>, type_list<FlagSet, FlagSets...>,
                                   type_list<CompleteFlags...>>
        : origanize_feature_flags<type_list<>, type_list<FlagSets...>,
                                  type_list<CompleteFlags..., placeholder>> {};

    template <class Flag, class... Flags, class... FlagOpts, template <class...> class FlagSet,
              class... FlagSets, class... CompleteFlags>
    struct origanize_feature_flags<type_list<Flag, Flags...>,
                                   type_list<FlagSet<FlagOpts...>, FlagSets...>,
                                   type_list<CompleteFlags...>>
        : std::conditional_t<
              wheel::type_traits::existing_v<Flag, FlagSet<FlagOpts...>>,
              origanize_feature_flags<type_list<Flags...>, type_list<FlagSets...>,
                                      type_list<CompleteFlags..., Flag>>,
              origanize_feature_flags<type_list<Flag, Flags...>, type_list<FlagSets...>,
                                      type_list<CompleteFlags..., placeholder>>> {};

    template <class Flag, class... Flags, class FlagOpt, class... FlagSets, class... CompleteFlags>
    struct origanize_feature_flags<type_list<Flag, Flags...>, type_list<FlagOpt, FlagSets...>,
                                   type_list<CompleteFlags...>>
        : std::conditional_t<
              std::is_same_v<Flag, FlagOpt>,
              origanize_feature_flags<type_list<Flags...>, type_list<FlagSets...>,
                                      type_list<CompleteFlags..., Flag>>,
              origanize_feature_flags<type_list<Flag, Flags...>, type_list<FlagSets...>,
                                      type_list<CompleteFlags..., placeholder>>> {};

    template <class... Flags, class FlagOpt, class... FlagSets, class... CompleteFlags>
    struct origanize_feature_flags<type_list<placeholder, Flags...>,
                                   type_list<FlagOpt, FlagSets...>, type_list<CompleteFlags...>>
        : origanize_feature_flags<type_list<Flags...>, type_list<FlagSets...>,
                                  type_list<CompleteFlags..., placeholder>> {};

    template <class FullFlag, class... Flags>
    using origanize_feature_flags_t = wheel::type_traits::copy_t<
        typename impl::origanize_feature_flags<type_list<Flags...>,
                                               wheel::type_traits::copy_t<FullFlag, type_list<>>,
                                               type_list<>>::type,
        FullFlag>;
  }  // namespace impl

  template <class FullFlag, class... Flags>
  using origanize_feature_flags_t = impl::origanize_feature_flags_t<FullFlag, Flags...>;

}  // namespace feature
XSL_NE
#endif
