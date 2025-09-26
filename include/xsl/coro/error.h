/**
 * @file error.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Async error handling utilities for the xsl library
 * @version 0.1.0
 * @date 2025-08-26
 *
 * @copyright Copyright (c) 2025
 *
 */

#pragma once
#ifndef XSL_CORO_ERROR
#  define XSL_CORO_ERROR
#  include <xsl/error.h>

#  define CO_TRV(var, expr, ...) __BASE__TRV(co_, var, expr, xsl::make_result_utils, __VA_ARGS__)

#  define CO_TRVEC(...) __MACRO_DISPATCH(__CO_TRVEC_, __VA_ARGS__)

#  define __CO_TRVEC_2(var, expr) __BASE__TRV(co_, var, expr, xsl::make_ec_utils)

#  define CO_ENSURE(...) __MACRO_DISPATCH(__ENSURE_, co_, __VA_ARGS__)
#  define CO_ENSURE2(...) __MACRO_DISPATCH(__ENSURE2_, co_, __VA_ARGS__)

#  define CO_ENSEC(expr, ...) __BASE__ENSURE(co_, expr, xsl::make_ec_utils, __VA_ARGS__)

#  define CO_RETURN(...) \
    co_return std::unexpected { xsl::ErrorUtil<>{}(__VA_ARGS__) }

#endif
