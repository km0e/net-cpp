#pragma once
#ifndef _XSL_FEATURE_H_
#  define _XSL_FEATURE_H_
#  include "xsl/def.h"
#  include "xsl/wheel/type_traits.h"

XSL_NAMESPACE_BEGIN
namespace feature {
  class placeholder;
  class node;
  template <class T>
  constexpr bool is_feature_flag_v
      = wheel::type_traits::existing_v<T, wheel::type_traits::_n<node, placeholder>>;
  template <class Flag>
  concept is_feature_flag
      = wheel::type_traits::existing_v<Flag, wheel::type_traits::_n<node, placeholder>>;

  namespace impl {
    template <class FeatureFlagSet, class FullFeatureFlagSet, class CompleteFeatureFlagSet>
    class origanize_feature_flags;

    template <is_feature_flag... Flags, is_feature_flag... CompleteFlags>
    class origanize_feature_flags<wheel::type_traits::_n<Flags...>, wheel::type_traits::_n<>,
                                  wheel::type_traits::_n<CompleteFlags...>>
        : public wheel::type_traits::_1<wheel::type_traits::_n<CompleteFlags...>> {};

    template <is_feature_flag... Flags, is_feature_flag Flag, is_feature_flag... FullFlags,
              class... CompleteFlags>
    class origanize_feature_flags<wheel::type_traits::_n<Flags...>,
                                  wheel::type_traits::_n<Flag, FullFlags...>,
                                  wheel::type_traits::_n<CompleteFlags...>>
        : public origanize_feature_flags<
              wheel::type_traits::_n<Flags...>, wheel::type_traits::_n<FullFlags...>,
              wheel::type_traits::_n<CompleteFlags...,
                                     std::conditional_t<wheel::type_traits::existing_v<
                                                            Flag, wheel::type_traits::_n<Flags...>>,
                                                        Flag, placeholder>>> {};
  }  // namespace impl

  template <class FeatureFlagSet, class FullFeatureFlagSet>
  using origanize_feature_flags_t
      = impl::origanize_feature_flags<FeatureFlagSet, FullFeatureFlagSet,
                                      wheel::type_traits::_n<>>::type;

}  // namespace feature
XSL_NAMESPACE_END
#endif
