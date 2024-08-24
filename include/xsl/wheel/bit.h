/**
 * @file bit.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Some bit manipulation functions.
 * @version 0.1
 * @date 2024-08-21
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_WHEEL_BIT
#  define XSL_WHEEL_BIT
#  include "xsl/wheel/def.h"

#  include <concepts>
XSL_WHEEL_NB
template <std::integral T>
constexpr T ceil2pow2(T n) {
  --n;
  for (auto i = 1u; i < sizeof(T) * 8; i <<= 1) {
    n |= n >> i;
  }
  return n + 1;
}
XSL_WHEEL_NE
#endif
