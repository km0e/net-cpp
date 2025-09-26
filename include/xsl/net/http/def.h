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
#ifndef XSL_NET_HTTP_DEF
#  define XSL_NET_HTTP_DEF

#  include <cstdint>
#  include <string_view>
#  include <xsl/net/def.h>

#  define XSL_NET_HTTP_NB \
    XSL_NET_NB            \
    namespace http {
#  define XSL_NET_HTTP_NE \
    }                     \
    XSL_NET_NE
XSL_NET_HTTP_NB
const std::string_view SERVER_VERSION = "XSL/0.1";
const uint16_t HTTP_DEFAULT_PORT = 80;
XSL_NET_HTTP_NE
#endif
