/**
 * @file macro.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Macro utilities for error handling and dispatching
 * @version 0.1.0
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

/*
 * @brief Deal with the expected with graceful error handling
 * @param ... variadic arguments
 * @example
 * TRY(expr) // expr must return std::expected<T, E>
 * TRY(expr, var) // expr must return std::expected<T, E>, var will be assigned the value of T
 */
#  define TRY(...) __MACRO_DISPATCH(__TRY_, __VA_ARGS__)

#  define TRV(...) __MACRO_DISPATCH(__TRV_, __VA_ARGS__)

#  define __BASE__TRY(co, expr, var, ret) \
    auto var = (expr);                    \
    if (!var) co##return ret;

#  define __BASE__TRV(co, expr, var, gen)                                         \
    auto __result_##var = (expr);                                                 \
    if (!__result_##var) co##return std::unexpected{gen(__result_##var.error())}; \
    auto& var = *__result_##var;

#  define __TRY_2(expr, var) __BASE__TRY(, expr, var, )
#  define __TRY_3(expr, var, err) __BASE__TRY(, expr, var, std::unexpected{err})

#  define __TRV_2(expr, var) __BASE__TRV(, expr, var, std::move)
#  define __TRV_3(expr, var, gen) __BASE__TRV(, expr, var, gen)

#  define ENSURE(...) __MACRO_DISPATCH(__ENSURE_, __VA_ARGS__)
#  define HNSURE(...) __MACRO_DISPATCH(__HNSURE_, __VA_ARGS__)

#  define __BASE__ENSURE(co, expr, ret) \
    do {                                \
      if (!(expr)) co##return ret;      \
    } while (0)

#  define __BASE__HNSURE(co, expr, gen)                                 \
    do {                                                                \
      auto __result = (expr);                                           \
      if (!__result) co##return std::unexpected{gen(__result.error())}; \
    } while (0)

#  define __ENSURE_1(expr) __BASE__ENSURE(, expr, )
#  define __ENSURE_2(expr, err) __BASE__ENSURE(, expr, std::unexpected{err})

#  define __HNSURE_1(expr) __BASE__HNSURE(co, expr, std::move)
#  define __HNSURE_2(expr, gen) __BASE__HNSURE(, expr, gen)

#  define EXPECT(...) __MACRO_DISPATCH(__EXPECT_, __VA_ARGS__)

#  define __BASE__EXPECT_3(co, expr, expected, gen)                        \
    do {                                                                   \
      auto __result = (expr);                                              \
      if (__result != expected) co##return std::unexpected{gen(__result)}; \
    } while (0)

#  define __EXPECT_3(expr, expected, gen) __BASE__EXPECT_3(, expr, expected, gen)

#  define EXPECT_N(expected, gen, e1, e2, ...) \
    do {                                       \
      auto __list = {e1, e2, __VA_ARGS__};     \
      for (auto& e : __list) {                 \
        __BASE__EXPECT_3(, e, expected, gen);  \
      }                                        \
    } while (0)

#  define MUST(...) __MACRO_DISPATCH(__MUST_, __VA_ARGS__)

#  define __BASE__MUST(expr)                    \
    do {                                        \
      auto __result_##var = (expr);             \
      if (!__result_##var) {                    \
        throw __result_##var.error().message(); \
      }                                         \
    } while (0)

#  define __BASE__MUST2(expr, var)            \
    auto __result_##var = (expr);             \
    if (!__result_##var) {                    \
      throw __result_##var.error().message(); \
    }                                         \
    auto& var = *__result_##var;

#  define __MUST_1(expr) __BASE__MUST(expr)
#  define __MUST_2(expr, var) __BASE__MUST2(expr, var)

XSL_NE
#endif
