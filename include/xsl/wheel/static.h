/**
 * @file static.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1
 * @date 2025-06-15
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once
#ifndef XSL_WHEEL_STATIC
#  define XSL_WHEEL_STATIC
#  include "xsl/wheel/def.h"

#  include <span>
XSL_WHEEL_NB
template <std::size_t value>
class StaticSize {
public:
  consteval StaticSize() = default;
  consteval operator std::size_t() const { return value; }
  consteval bool is_dynamic() { return false; }
};

template <>
class StaticSize<std::dynamic_extent> {
  std::size_t value;

public:
  constexpr StaticSize(std::size_t value) : value(value) {}
  constexpr operator std::size_t() const { return value; }
  constexpr bool is_dynamic() { return true; }
};

XSL_WHEEL_NE
#endif
