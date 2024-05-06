#pragma once
#ifndef _XSL_NET_HTTP_ROUTER_H_
#define _XSL_NET_HTTP_ROUTER_H_
#include <xsl/http/config.h>
#include <xsl/http/http_msg.h>
#include <xsl/utils/wheel/wheel.h>

HTTP_NAMESPACE_BEGIN

using HttpRouteHandler = wheel::function<wheel::string(HttpRequest&)>;

enum class AddRouteErrorKind {
  Unknown,
  InvalidPath,
  Conflict,
};

class AddRouteError {
public:
  AddRouteError(AddRouteErrorKind kind);
  AddRouteError(AddRouteErrorKind kind, wheel::string message);
  ~AddRouteError();
  AddRouteErrorKind kind;
  wheel::string message;
};

enum class RouteErrorKind {
  Unknown,
  NotFound,
  InvalidPath,
};

class RouteError {
public:
  RouteError(RouteErrorKind kind);
  RouteError(RouteErrorKind kind, wheel::string message);
  ~RouteError();
  RouteErrorKind kind;
  wheel::string message;
};

using AddRouteResult = wheel::Result<wheel::tuple<>, AddRouteError>;
using RouteResult = wheel::Result<HttpRouteHandler, RouteError>;

namespace router_details {
  class HttpRouteNode {
  public:
    HttpRouteNode();
    ~HttpRouteNode();
    AddRouteResult add_route(wheel::string_view path, HttpRouteHandler handler);
    RouteResult route(HttpRequest& request);
  private:
    wheel::unordered_map<wheel::string_view, HttpRouteHandler> handlers;
    wheel::unordered_map<wheel::string_view, wheel::shared_ptr<HttpRouteNode>> children;
  };
}  // namespace router_details

class HttpRouter {
public:
  HttpRouter();
  ~HttpRouter();
  void add_route(wheel::string_view path, HttpRouteHandler handler);
  wheel::string route(HttpRequest& request);
private:
  router_details::HttpRouteNode root;
};
HTTP_NAMESPACE_END
#endif
