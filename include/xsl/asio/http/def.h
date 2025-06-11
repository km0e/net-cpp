/**
 * @file def.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-09-01
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_ASIO_HTTP_DEF
#  define XSL_ASIO_HTTP_DEF
#  include <string_view>

#  define XSL_ASIO_HTTP_NB namespace xsl::_asio::http {
#  define XSL_ASIO_HTTP_NE }
XSL_ASIO_HTTP_NB
const std::string_view SERVER_VERSION = "XSL/0.1";
namespace tag {
  struct skt;
  struct dev;
}  // namespace tag
XSL_ASIO_HTTP_NE
#endif
