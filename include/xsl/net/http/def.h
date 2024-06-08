#pragma once
#ifndef _XSL_NET_HTTP_DEF_H_
#  define _XSL_NET_HTTP_DEF_H_
#  include <string_view>

#  define HTTP_NAMESPACE_BEGIN namespace xsl::net::http {
#  define HTTP_NAMESPACE_END }
HTTP_NAMESPACE_BEGIN
const std::string_view SERVER_VERSION = "XSL/0.1";
HTTP_NAMESPACE_END
#endif
