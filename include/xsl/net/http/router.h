#pragma once
#ifndef _XSL_NET_HTTP_ROUTER_H_
#  define _XSL_NET_HTTP_ROUTER_H_
#  include "xsl/net/http/context.h"
#  include "xsl/net/http/def.h"
#  include "xsl/net/http/msg.h"
#  include "xsl/wheel.h"

HTTP_NAMESPACE_BEGIN

class RouteHandleError {
public:
  RouteHandleError();
  RouteHandleError(string message);
  ~RouteHandleError();
  string message;
  string to_string() const;
};

using RouteHandleResult = Result<IntoSendTasksPtr, RouteHandleError>;

using RouteHandler = function<RouteHandleResult(Context& ctx)>;

enum class AddRouteErrorKind {
  Unknown,
  InvalidPath,
  Conflict,
};
const int ADD_ROUTE_ERROR_COUNT = 3;
const array<string_view, ADD_ROUTE_ERROR_COUNT> ADD_ROUTE_ERROR_STRINGS = {
    "Unknown",
    "InvalidPath",
    "Conflict",
};
class AddRouteError {
public:
  AddRouteError(AddRouteErrorKind kind);
  AddRouteError(AddRouteErrorKind kind, string message);
  ~AddRouteError();
  AddRouteErrorKind kind;
  string message;
  std::string to_string() const;
};

enum class RouteErrorKind : uint8_t {
  Unknown,
  NotFound,
  Unimplemented,
};
const int ROUTE_ERROR_COUNT = 3;
const array<string_view, ROUTE_ERROR_COUNT> ROUTE_ERROR_STRINGS = {
    "Unknown",
    "NotFound",
    "Unimplemented",
};
class RouteError {
public:
  RouteError();
  RouteError(RouteErrorKind kind);
  RouteError(RouteErrorKind kind, string message);
  ~RouteError();
  RouteErrorKind kind;
  string message;
  string to_string() const;
};

using AddRouteResult = Result<tuple<>, AddRouteError>;

using RouteResult = Result<IntoSendTasksPtr, RouteError>;

template <class R>
concept Router = requires(R r, string_view path, RouteHandler&& handler, Context& ctx) {
  { r.add_route(HttpMethod{}, path, xsl::move(handler)) } -> same_as<AddRouteResult>;
  { r.route(ctx) } -> same_as<RouteResult>;
  { r.error_handler(RouteErrorKind{}, xsl::move(handler)) };
};

namespace router_details {
  class HttpRouteNode {
  public:
    HttpRouteNode();
    ~HttpRouteNode();
    AddRouteResult add_route(HttpMethod method, string_view path, RouteHandler&& handler);
    RouteResult route(Context& ctx);

  private:
    array<unique_ptr<RouteHandler>, METHOD_COUNT> handlers;
    ConcurrentHashMap<string_view, shared_ptr<HttpRouteNode>> children;
  };
}  // namespace router_details

const RouteHandler UNKNOWN_HANDLER = []([[maybe_unused]] Context& ctx) -> RouteHandleResult {
  return RouteHandleResult{
      std::make_unique<ResponsePart>(500, "Internal Server Error", HttpVersion::HTTP_1_1)};
};

const RouteHandler NOT_FOUND_HANDLER = []([[maybe_unused]] Context& ctx) -> RouteHandleResult {
  return RouteHandleResult{std::make_unique<ResponsePart>(404, "Not Found", HttpVersion::HTTP_1_1)};
};

const RouteHandler UNIMPLEMENTED_HANDLER = []([[maybe_unused]] Context& ctx) -> RouteHandleResult {
  return RouteHandleResult{
      std::make_unique<ResponsePart>(501, "Not Implemented", HttpVersion::HTTP_1_1)};
};

class HttpRouter {
public:
  HttpRouter();
  ~HttpRouter();
  AddRouteResult add_route(HttpMethod method, string_view path, RouteHandler&& handler);
  RouteResult route(Context& ctx);
  void error_handler(RouteErrorKind kind, RouteHandler&& handler);

private:
  router_details::HttpRouteNode root;
  array<shared_ptr<RouteHandler>, ROUTE_ERROR_COUNT> error_handlers
      = {make_shared<RouteHandler>(UNKNOWN_HANDLER), make_shared<RouteHandler>(NOT_FOUND_HANDLER),
         make_shared<RouteHandler>(UNIMPLEMENTED_HANDLER)};
};

HTTP_NAMESPACE_END
#endif
