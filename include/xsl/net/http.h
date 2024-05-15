#pragma once
#ifndef _XSL_NET_HTTP_CONFIG_H_
#  define _XSL_NET_HTTP_CONFIG_H_
#  include "xsl/net/def.h"
#  include "xsl/net/http/def.h"
#  include "xsl/net/http/helper.h"
#  include "xsl/net/http/msg.h"
#  include "xsl/net/http/parse.h"
#  include "xsl/net/http/router.h"
#  include "xsl/net/http/server.h"
NET_NAMESPACE_BEGIN
using http::detail::create_static_handler;
using http::detail::DefaultResponse;
using http::detail::HttpHandlerGenerator;
using http::detail::HttpMethod;
using http::detail::HttpParser;
using http::detail::HttpRouter;
using http::detail::HttpServer;
using http::detail::HttpServerConfig;
using http::detail::HttpVersion;
using http::detail::Request;
using http::detail::ResponsePart;
using http::detail::RouteErrorKind;
using http::detail::StaticCreateResult;
NET_NAMESPACE_END
#endif
