/**
 * @file dyn.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-09-01
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_DYN
#  define XSL_DYN
#  include "xsl/def.h"
#  include "xsl/type_traits.h"

#  include <memory>
#  include <type_traits>
XSL_NB

namespace {
  template <class T>
  struct DynGet {
    using type = typename T::dynamic_type;
  };

  template <class T, class U>
  struct dyn_cast_pred : std::false_type {};

  template <class T, class U>
    requires std::is_same_v<T, typename U::dynamic_type>
  struct dyn_cast_pred<T, U> : std::true_type {};

}  // namespace

template <template <class> class GetChain, class From, class To>
constexpr bool dyn_cast_v = existing_v<To, for_each_t<DynGet, typename GetChain<From>::type>>;

template <template <class> class GetChain, class Dyn>
constexpr decltype(auto) to_dyn_unique(auto&& t) {
  using raw_type = std::decay_t<decltype(t)>;
  if constexpr (std::is_same_v<raw_type, Dyn>) {
    return std::addressof(t);
  } else {
    return std::unique_ptr<Dyn>{std::make_unique<
        typename remove_first_if<typename GetChain<raw_type>::type, dyn_cast_pred, Dyn>::type2>(
        std::forward<decltype(t)>(t))};
  }
}
XSL_NE
#endif
