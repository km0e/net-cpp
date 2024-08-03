#pragma once
#ifndef XSL_NET_HTTP_DEF
#  define XSL_NET_HTTP_DEF
#  include <string_view>

#  define HTTP_NB namespace xsl::_net::http {
#  define HTTP_NE }
HTTP_NB
const std::string_view SERVER_VERSION = "XSL/0.1";
HTTP_NE
#endif
