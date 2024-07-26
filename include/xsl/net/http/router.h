#pragma once
#ifndef XSL_NET_HTTP_ROUTER
#  define XSL_NET_HTTP_ROUTER
#  include "xsl/net/http/def.h"
#  include "xsl/net/http/msg.h"
#  include "xsl/net/http/proto.h"
#  include "xsl/wheel.h"

#  include <expected>
#  include <string_view>

HTTP_NB

class RouteContext {
public:
  RouteContext(Request&& request);
  ~RouteContext();
  std::string_view current_path;
  Request request;
};

class RouteHandleError {
public:
  RouteHandleError();
  RouteHandleError(std::string message);
  ~RouteHandleError();
  std::string message;
  std::string to_string() const;
};

using RouteHandleResult = std::expected<HttpResponse, RouteHandleError>;

using RouteHandler = std::function<RouteHandleResult(RouteContext& ctx)>;

enum class AddRouteErrorKind {
  Unknown,
  InvalidPath,
  Conflict,
};
const int ADD_ROUTE_ERROR_COUNT = 3;
const std::array<std::string_view, ADD_ROUTE_ERROR_COUNT> ADD_ROUTE_ERROR_STRINGS = {
    "Unknown",
    "InvalidPath",
    "Conflict",
};
class AddRouteError {
public:
  AddRouteError(AddRouteErrorKind kind);
  AddRouteError(AddRouteErrorKind kind, std::string message);
  ~AddRouteError();
  AddRouteErrorKind kind;
  std::string message;
  std::string to_string() const;
};

enum class RouteErrorKind : uint8_t {
  Unknown,
  NotFound,
  Unimplemented,
};
const int ROUTE_ERROR_COUNT = 3;
const std::array<std::string_view, ROUTE_ERROR_COUNT> ROUTE_ERROR_STRINGS = {
    "Unknown",
    "NotFound",
    "Unimplemented",
};
class RouteError {
public:
  RouteError();
  RouteError(RouteErrorKind kind);
  RouteError(RouteErrorKind kind, std::string message);
  ~RouteError();
  RouteErrorKind kind;
  std::string message;
  std::string to_string() const;
};

using AddRouteResult = std::expected<void, AddRouteError>;

using RouteResult = std::expected<HttpResponse, RouteError>;

template <class R>
concept Router = requires(R r, HttpMethod hm, std::string_view path, RouteHandler&& handler,
                          RouteContext& ctx) {
  { r.add_route(hm, path, std::move(handler)) } -> std::same_as<AddRouteResult>;
  { r.route(ctx) } -> std::same_as<RouteResult>;
  { r.error_handler(RouteErrorKind{}, std::move(handler)) };
};

namespace router_details {
  class HttpRouteNode {
  public:
    HttpRouteNode();
    HttpRouteNode(HttpMethod method, RouteHandler&& handler);
    ~HttpRouteNode();
    AddRouteResult add_route(HttpMethod method, std::string_view path, RouteHandler&& handler);
    RouteResult route(RouteContext& ctx);

  private:
    std::array<RouteHandler, HTTP_METHOD_COUNT> handlers;
    ShrdRes<sumap<HttpRouteNode>> children;

    bool add(HttpMethod method, RouteHandler&& handler);
    RouteResult direct_route(RouteContext& ctx);
  };
}  // namespace router_details

const RouteHandler UNKNOWN_HANDLER = []([[maybe_unused]] RouteContext& ctx) -> RouteHandleResult {
  return RouteHandleResult{ResponsePart(HttpVersion::HTTP_1_1, 500, "Internal Server Error")};
};

const RouteHandler NOT_FOUND_HANDLER = []([[maybe_unused]] RouteContext& ctx) -> RouteHandleResult {
  return RouteHandleResult{ResponsePart(HttpVersion::HTTP_1_1, 404, "Not Found")};
};

const RouteHandler UNIMPLEMENTED_HANDLER
    = []([[maybe_unused]] RouteContext& ctx) -> RouteHandleResult {
  return RouteHandleResult{ResponsePart(HttpVersion::HTTP_1_1, 501, "Not Implemented")};
};

class HttpRouter {
public:
  HttpRouter();
  ~HttpRouter();
  AddRouteResult add_route(HttpMethod method, std::string_view path, RouteHandler&& handler);
  RouteResult route(RouteContext& ctx);
  void error_handler(RouteErrorKind kind, RouteHandler&& handler);

private:
  router_details::HttpRouteNode root;
  std::array<std::shared_ptr<RouteHandler>, ROUTE_ERROR_COUNT> error_handlers
      = {make_shared<RouteHandler>(UNKNOWN_HANDLER), make_shared<RouteHandler>(NOT_FOUND_HANDLER),
         make_shared<RouteHandler>(UNIMPLEMENTED_HANDLER)};
};

HTTP_NE
#endif
