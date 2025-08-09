/**
 * @file def.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Definitions and utilities for the xsl library
 * @version 0.2.0
 * @date 2024-09-01
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_DEF
#  define XSL_DEF
#  include <expected>
#  include <functional>
#  include <optional>
#  include <system_error>

#  define XSL_NB namespace xsl {
#  define XSL_NE }

XSL_NB
using std::errc;

#  ifndef __cpp_lib_move_only_function
template <class F>
using move_only_function = std::function<F>;
#  else
using std::move_only_function;
#  endif  // __cpp_lib_move_only_function

using std::expected;
using std::optional;

XSL_NE
#endif
