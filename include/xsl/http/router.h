#pragma once
#ifndef _XSL_NET_HTTP_ROUTER_H_
#  define _XSL_NET_HTTP_ROUTER_H_
#  include "xsl/http/context.h"
#  include "xsl/http/def.h"
#  include "xsl/http/msg.h"
#  include "xsl/wheel/wheel.h"

#  include <cstdint>
HTTP_NAMESPACE_BEGIN

class RouteHandleError {
public:
  RouteHandleError();
  RouteHandleError(wheel::string message);
  ~RouteHandleError();
  wheel::string message;
  wheel::string to_string() const;
};

using RouteHandleResult = wheel::Result<IntoSendTasksPtr, RouteHandleError>;

using RouteHandler = wheel::function<RouteHandleResult(Context& ctx)>;

enum class AddRouteErrorKind {
  Unknown,
  InvalidPath,
  Conflict,
};
const int ADD_ROUTE_ERROR_COUNT = 3;
const wheel::array<wheel::string_view, ADD_ROUTE_ERROR_COUNT> ADD_ROUTE_ERROR_STRINGS = {
    "Unknown",
    "InvalidPath",
    "Conflict",
};
class AddRouteError {
public:
  AddRouteError(AddRouteErrorKind kind);
  AddRouteError(AddRouteErrorKind kind, wheel::string message);
  ~AddRouteError();
  AddRouteErrorKind kind;
  wheel::string message;
  std::string to_string() const;
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

using RouteResult = wheel::Result<IntoSendTasksPtr, RouteError>;

template <class R>
concept Router = requires(R r, wheel::string_view path, RouteHandler&& handler, Context& ctx) {
  { r.add_route(HttpMethod{}, path, wheel::move(handler)) } -> wheel::same_as<AddRouteResult>;
  { r.route(ctx) } -> wheel::same_as<RouteResult>;
  { r.error_handler(RouteErrorKind{}, wheel::move(handler)) };
};

namespace router_details {
  class HttpRouteNode {
  public:
    HttpRouteNode();
    ~HttpRouteNode();
    AddRouteResult add_route(HttpMethod method, wheel::string_view path, RouteHandler&& handler);
    RouteResult route(Context& ctx);

  private:
    wheel::array<wheel::unique_ptr<RouteHandler>, METHOD_COUNT> handlers;
    wheel::ConcurrentHashMap<wheel::string_view, wheel::shared_ptr<HttpRouteNode>> children;
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

class DefaultRouter {
public:
  DefaultRouter();
  ~DefaultRouter();
  AddRouteResult add_route(HttpMethod method, wheel::string_view path, RouteHandler&& handler);
  RouteResult route(Context& ctx);
  void error_handler(RouteErrorKind kind, RouteHandler&& handler);

private:
  router_details::HttpRouteNode root;
  wheel::array<wheel::shared_ptr<RouteHandler>, ROUTE_ERROR_COUNT> error_handlers
      = {wheel::make_shared<RouteHandler>(UNKNOWN_HANDLER),
         wheel::make_shared<RouteHandler>(NOT_FOUND_HANDLER),
         wheel::make_shared<RouteHandler>(UNIMPLEMENTED_HANDLER)};
};

HTTP_NAMESPACE_END
#endif
