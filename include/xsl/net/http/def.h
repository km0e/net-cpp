#pragma once
#ifndef XSL_NET_HTTP_DEF
#  define XSL_NET_HTTP_DEF
#  include <string_view>

#  define XSL_HTTP_NB namespace xsl::_net::http {
#  define XSL_HTTP_NE }
XSL_HTTP_NB
const std::string_view SERVER_VERSION = "XSL/0.1";
XSL_HTTP_NE
#endif
