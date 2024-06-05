#pragma once
#ifndef _XSL_NET_HTTP_HELPER_STATIC_H_
#  define _XSL_NET_HTTP_HELPER_STATIC_H_
#  include "xsl/net/http/component/def.h"
#  include "xsl/net/http/router.h"
#  include "xsl/wheel.h"
HTTP_HELPER_NAMESPACE_BEGIN

using StaticCreateResult = Result<http::RouteHandler, http::AddRouteError>;

// @brief Create a static handler for a file or folder.
// @param path The path of the file or folder.
// @return A static handler for the file or folder.

StaticCreateResult create_static_handler(std::string&& path);

HTTP_HELPER_NAMESPACE_END
#endif  // _XSL_NET_HTTP_HELPER_STATIC_H_
