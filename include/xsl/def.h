/**
 * @file def.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.11
 * @date 2024-09-01
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_DEF
#  define XSL_DEF
#  include <cstdint>
#  include <functional>
#  define XSL_NB namespace xsl {
#  define XSL_NE }
XSL_NB
using byte = std::uint8_t;

#  ifndef __cpp_lib_move_only_function
template <class F>
using move_only_function = std::function<F>;
#  else
using std::move_only_function;
#  endif  // __cpp_lib_move_only_function

XSL_NE
#endif
