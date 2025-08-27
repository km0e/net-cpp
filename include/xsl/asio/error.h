/**
 * @file error.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Asio error handling utilities for the xsl library
 * @version 0.1.0
 * @date 2025-08-26
 *
 * @copyright Copyright (c) 2025
 *
 */

#pragma once
#ifndef XSL_ASIO_ERROR
#  define XSL_ASIO_ERROR
#  include <xsl/asio/def.h>
#  include <xsl/coro/error.h>
#  include <xsl/error.h>
#  include <xsl/sys.h>

XSL_ASIO_NB
XSL_ASIO_NE

using xsl::sys::net::GaiErrorUtil;

#  define CO_TRVGAI(var, expr) CO_TRV(var, expr, GaiErrorUtil{})

#endif
