/**
 * @file ptr.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-09-01
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_WHEEL_PTR
#  define XSL_WHEEL_PTR
#  include "xsl/wheel/def.h"

#  include <memory>
XSL_WHEEL_NB
template <typename Ptr>
concept PtrLike = requires(Ptr ptr) {
  std::pointer_traits<Ptr>::element_type;
  std::pointer_traits<Ptr>::pointer;
  std::pointer_traits<Ptr>::difference_type;
};
XSL_WHEEL_NE
#endif
