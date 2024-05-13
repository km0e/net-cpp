#pragma once
#ifndef _XSL_NET_HTTP_HELPER_STATIC_H_
#  define _XSL_NET_HTTP_HELPER_STATIC_H_
#  include "xsl/http/helper/def.h"
#  include "xsl/http/router.h"
#  include "xsl/wheel/wheel.h"
HELPER_NAMESPACE_BEGIN


using StaticCreateResult = wheel::Result<http::detail::RouteHandler, http::detail::AddRouteError>;

StaticCreateResult create_static_handler(wheel::string&& path);

HELPER_NAMESPACE_END
#endif  // _XSL_NET_HTTP_HELPER_STATIC_H_
