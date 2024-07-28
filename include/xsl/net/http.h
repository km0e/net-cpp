#pragma once
#ifndef XSL_NET_HTTP
#  define XSL_NET_HTTP
#  include "xsl/net/def.h"
#  include "xsl/net/http/component.h"
#  include "xsl/net/http/msg.h"
#  include "xsl/net/http/parse.h"
#  include "xsl/net/http/router.h"
#  include "xsl/net/http/server.h"
NET_NB
using http::create_static_handler;
using http::HttpMethod;
using http::HttpParser;
using http::HttpRouter;
// using http::HttpServer;
using HttpRequestView = http::RequestView;
using http::HTTP_METHOD_STRINGS;
using http::HttpVersion;
using HttpRouteHandleResult = http::RouteHandleResult;
using HttpRouteResult = http::RouteResult;
using HttpRouteContext = http::RouteContext;
using http::Request;
using http::to_string_view;
using HttpResponsePart = http::ResponsePart;
using http::HttpResponse;
using http::RouteError;
using http::StaticCreateResult;
using HttpParseResult = http::ParseResult;
NET_NE
#endif
