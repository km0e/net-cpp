#include <spdlog/spdlog.h>
#include <xsl/http/config.h>
#include <xsl/http/http_msg.h>
#include <xsl/http/http_router.h>
#include <xsl/utils/wheel/wheel.h>

#include "xsl/http/http_server.h"

HTTP_NAMESPACE_BEGIN
AddRouteError::AddRouteError(AddRouteErrorKind kind) : kind(kind) {}
AddRouteError::AddRouteError(AddRouteErrorKind kind, wheel::string message)
    : kind(kind), message(message) {}
AddRouteError::~AddRouteError() {}

RouteError::RouteError(RouteErrorKind kind) : kind(kind) {}
RouteError::RouteError(RouteErrorKind kind, wheel::string message) : kind(kind), message(message) {}
RouteError::~RouteError() {}

HttpParser::HttpParser() {}

HttpParser::~HttpParser() {}

namespace router_details {
  HttpRouteNode::HttpRouteNode() {}
  HttpRouteNode::~HttpRouteNode() {}

  AddRouteResult HttpRouteNode::add_route(wheel::string_view path, HttpRouteHandler handler) {
    spdlog::debug("[HttpRouteNode::add_route] Adding route: {}", path);
    if (path[0] != '/') {
      return AddRouteResult(AddRouteError(AddRouteErrorKind::InvalidPath, ""));
    }
    auto pos = path.find('/', 1);
    if (pos == wheel::string_view::npos) {
      path = path.substr(1);
      auto res = handlers.try_emplace(path, handler);
      if (!res.second) {
        return AddRouteResult(AddRouteError(AddRouteErrorKind::Conflict, ""));
      }
      spdlog::debug("[HttpRouteNode::add_route] Successfully added route: {}", path);
      return AddRouteResult({});
    }
    auto res = children.try_emplace(path.substr(1, pos - 1), wheel::make_shared<HttpRouteNode>());
    return res.first->second->add_route(path.substr(pos), handler);
  }

  RouteResult HttpRouteNode::route(HttpRequest& request) {
    spdlog::debug("[HttpRouteNode::route] Routing request: {}", request.path);
    if (request.path[0] != '/') {
      return RouteResult(RouteError(RouteErrorKind::InvalidPath, ""));
    }
    auto pos = request.path.find('/', 1);
    if (pos == wheel::string_view::npos) {
      auto it = handlers.find(request.path.substr(1));
      if (it != handlers.end()) {
        return RouteResult(it->second);
      }
      it = handlers.find("");
      if (it != handlers.end()) {
        return RouteResult(it->second);
      }
      return RouteResult(RouteError(RouteErrorKind::NotFound, ""));
    }
    auto it = children.find(request.path.substr(1, pos - 1));
    if (it != children.end()) {
      return it->second->route(request);
    }
    it = children.find("");
    if (it != children.end()) {
      return it->second->route(request);
    }
    return RouteResult(RouteError(RouteErrorKind::NotFound, ""));
  }
}  // namespace router_details

HttpRouter::HttpRouter() : root() {}

HttpRouter::~HttpRouter() {}

void HttpRouter::add_route(wheel::string_view path, HttpRouteHandler handler) {
  spdlog::debug("[HttpRouter::add_route] Adding route: {}", path);
  auto res = root.add_route(path, handler);
  if (res.is_err()) {
    spdlog::error("[HttpRouter::add_route] Failed to add route: {}", res.unwrap_err().message);
  }
}

wheel::string HttpRouter::route(HttpRequest& request) {
  spdlog::debug("[HttpRouter::route] Routing request: {}", request.path);
  auto res = root.route(request);
  if (res.is_err()) {
    spdlog::error("[HttpRouter::route] Failed to route: {}", res.unwrap_err().message);
    return "";
  }
  return res.unwrap()(request);
}

HTTP_NAMESPACE_END
