/**
 * @file compress.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-09-01
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_NET_HTTP_COMPONENT_COMPRESS
#  define XSL_NET_HTTP_COMPONENT_COMPRESS
#  include "xsl/net/http/def.h"
#  include "xsl/wheel.h"

#  include <string_view>
XSL_HTTP_NB
const us_map<std::string_view> encoding_to_extension = {
    {"br", ".br"},
    {"gzip", ".gz"},

    {"deflate", ".deflate"},
};
XSL_HTTP_NE
#endif
