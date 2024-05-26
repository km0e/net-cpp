#pragma once
#ifndef _XSL_NET_HTTP_HELPER_STATIC_H_
#  define _XSL_NET_HTTP_HELPER_STATIC_H_
#  include "xsl/net/http/helper/def.h"
#  include "xsl/net/http/router.h"
#  include "xsl/wheel.h"
HTTP_HELPER_NAMESPACE_BEGIN

using StaticCreateResult = Result<http::RouteHandler, http::AddRouteError>;

StaticCreateResult create_static_handler(string&& path);

HTTP_HELPER_NAMESPACE_END
#endif  // _XSL_NET_HTTP_HELPER_STATIC_H_
