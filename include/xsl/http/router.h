#pragma once
#ifndef _XSL_NET_HTTP_ROUTER_H_
#  define _XSL_NET_HTTP_ROUTER_H_
#  include <xsl/http/http.h>
#  include <xsl/http/msg.h>
#  include <xsl/utils/wheel/wheel.h>

#  include <cstdint>
HTTP_NAMESPACE_BEGIN

using HttpRouteHandler = wheel::function<HttpResponse(HttpRequest&&)>;

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

enum class RouteErrorKind : uint8_t {
  Unknown,
  NotFound,
  Unimplemented,
};
const int ROUTE_ERROR_COUNT = 3;
const wheel::array<wheel::string_view, ROUTE_ERROR_COUNT> ROUTE_ERROR_STRINGS = {
    "Unknown",
    "NotFound",
    "Unimplemented",
};
class RouteError {
public:
  RouteError();
  RouteError(RouteErrorKind kind);
  RouteError(RouteErrorKind kind, wheel::string message);
  ~RouteError();
  RouteErrorKind kind;
  wheel::string message;
  wheel::string to_string() const;
};

using AddRouteResult = wheel::Result<wheel::tuple<>, AddRouteError>;
using RouteResult = wheel::Result<wheel::shared_ptr<HttpRouteHandler>, RouteError>;

template <class R>
concept Router
    = requires(R r, wheel::string_view path, HttpRouteHandler&& handler, HttpRequest& request) {
        { r.add_route(HttpMethod{}, path, wheel::move(handler)) } -> wheel::same_as<AddRouteResult>;
        { r.route(request) } -> wheel::same_as<RouteResult>;
        { r.error_handler(RouteError{}, wheel::move(handler)) };
      };

namespace router_details {
  class HttpRouteNode {
  public:
    HttpRouteNode();
    ~HttpRouteNode();
    AddRouteResult add_route(HttpMethod method, wheel::string_view path,
                             HttpRouteHandler&& handler);
    RouteResult route(HttpRequest& request);

  private:
    wheel::array<wheel::shared_ptr<HttpRouteHandler>, METHOD_COUNT> handlers;
    wheel::unordered_map<wheel::string_view, wheel::shared_ptr<HttpRouteNode>> children;
  };
}  // namespace router_details

const HttpRouteHandler UNKNOWN_HANDLER = [](HttpRequest&& request) -> HttpResponse {
  return HttpResponse{"HTTP/1.1", 500, "Internal Server Error"};
};

const HttpRouteHandler NOT_FOUND_HANDLER = [](HttpRequest&& request) -> HttpResponse {
  return HttpResponse{"HTTP/1.1", 404, "Not Found"};
};
const HttpRouteHandler UNIMPLEMENTED_HANDLER = [](HttpRequest&& request) -> HttpResponse {
  return HttpResponse{"HTTP/1.1", 501, "Not Implemented"};
};
class DefaultRouter {
public:
  DefaultRouter();
  ~DefaultRouter();
  AddRouteResult add_route(HttpMethod method, wheel::string_view path, HttpRouteHandler&& handler);
  RouteResult route(HttpRequest& request);
  void error_handler(RouteError error, HttpRouteHandler&& handler);

private:
  router_details::HttpRouteNode root;
  wheel::array<wheel::shared_ptr<HttpRouteHandler>, ROUTE_ERROR_COUNT> error_handlers
      = {wheel::make_shared<HttpRouteHandler>(UNKNOWN_HANDLER),
         wheel::make_shared<HttpRouteHandler>(NOT_FOUND_HANDLER),
         wheel::make_shared<HttpRouteHandler>(UNIMPLEMENTED_HANDLER)};
};

HTTP_NAMESPACE_END
#endif
