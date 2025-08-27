/**
 * @file macro.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Macro utilities for error handling and dispatching
 * @version 0.1.2
 * @date 2025-08-05
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_MACRO
#  define XSL_MACRO
#  include <xsl/def.h>

XSL_NB
#  define __COUNT_ARGS(_1, _2, _3, _4, _5, _6, _7, _8, COUNT, ...) COUNT
#  define __NUM_ARGS(...) __COUNT_ARGS(__VA_ARGS__, 8, 7, 6, 5, 4, 3, 2, 1)

#  ifndef __CONCAT
#    define __CONCAT(x, y) x##y
#  endif

#  define __CONCAT2(x, y) __CONCAT(x, y)

/*
 * @brief Dispatch to the correct macro based on the number of arguments
 * @param ... variadic arguments
 * @return the result of the macro with the correct number of arguments
 * @example
 * # define TEST(...) __MACRO_DISPATCH(TEST, __VA_ARGS__)
 * # define TEST1(a) a
 * # define TEST2(a, b) a + b
 *
 * TEST(1) // expands to TEST1(1)
 * TEST(1, 2) // expands to TEST2(1, 2)
 *
 * */
#  define __MACRO_DISPATCH(pre, ...) __CONCAT2(pre, __NUM_ARGS(__VA_ARGS__))(__VA_ARGS__)

#  define __IGNORE_1(a, ...) (__VA_ARGS__)

#  define __WRAP_LAMBDA(...) [&]() { return (__VA_ARGS__); }

#  define __MAP_N(macro, ...) __MACRO_DISPATCH(__MAP_, macro, __VA_ARGS__)

#  define __MAP_2(macro, a) macro(a)
#  define __MAP_3(macro, a, ...) macro(a), __MAP_2(macro, __VA_ARGS__)
#  define __MAP_4(macro, a, ...) macro(a), __MAP_3(macro, __VA_ARGS__)
#  define __MAP_5(macro, a, ...) macro(a), __MAP_4(macro, __VA_ARGS__)
#  define __MAP_6(macro, a, ...) macro(a), __MAP_5(macro, __VA_ARGS__)

XSL_NE
#endif
