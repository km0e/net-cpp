#pragma once
#ifndef XSL_WHEEL_PTR
#  define XSL_WHEEL_PTR
#  include "xsl/wheel/def.h"

#  include <concepts>
#  include <type_traits>
XSL_WHEEL_NB
template <typename Ptr>
struct PtrTraits {
  using element_type = typename Ptr::element_type;
};

template <typename T>
struct PtrTraits<T*> {
  using element_type = T;
};

template <typename Ptr, typename T = typename PtrTraits<Ptr>::element_type>
concept PtrLike = std::is_pointer_v<Ptr> || requires(Ptr ptr) {
  { *ptr } -> std::same_as<typename PtrTraits<Ptr>::element_type&>;
  { ptr.operator->() } -> std::same_as<typename PtrTraits<Ptr>::element_type*>;
};
XSL_WHEEL_NE
#endif
