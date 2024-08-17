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
