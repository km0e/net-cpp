/**
 * @file error.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Network error handling utilities for the xsl library
 * @version 0.1.0
 * @date 2025-08-23
 *
 * @copyright Copyright (c) 2025
 *
 */

#pragma once
#ifndef XSL_SYS_NET_ERROR
#  define XSL_SYS_NET_ERROR
#  include <netdb.h>
#  include <xsl/error.h>
#  include <xsl/sys/net/def.h>

XSL_SYS_NET_NB

struct GaiError : public Error {
  using Error::Error;
  ~GaiError() override = default;
  auto message() const -> std::string_view override { return gai_strerror(code()); }
};
XSL_SYS_NET_NE

#endif
