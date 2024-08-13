#pragma once
#ifndef XSL_NET_HTTP_COMPONENT_COMPRESS
#  define XSL_NET_HTTP_COMPONENT_COMPRESS
#  include "xsl/net/http/component/def.h"
#  include "xsl/wheel.h"

#  include <string_view>
XSL_NET_HTTP_COMPONENT_NB
const us_map<std::string_view> encoding_to_extension = {
  {"br", ".br"},
  {"gzip", ".gz"},
  
  {"deflate", ".deflate"},
};
XSL_NET_HTTP_COMPONENT_NE
#endif
