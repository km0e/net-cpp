#include <spdlog/spdlog.h>
#include <xsl/http/http.h>
#include <xsl/http/msg.h>
#include <xsl/http/router.h>
#include <xsl/http/server.h>
#include <xsl/utils/wheel/wheel.h>

#include <cstdint>

#include "xsl/http/context.h"

HTTP_NAMESPACE_BEGIN
AddRouteError::AddRouteError(AddRouteErrorKind kind) : kind(kind) {}
AddRouteError::AddRouteError(AddRouteErrorKind kind, wheel::string message)
    : kind(kind), message(message) {}
AddRouteError::~AddRouteError() {}

RouteError::RouteError() {}
RouteError::RouteError(RouteErrorKind kind) : kind(kind) {}
RouteError::RouteError(RouteErrorKind kind, wheel::string message) : kind(kind), message(message) {}
RouteError::~RouteError() {}
wheel::string RouteError::to_string() const {
  return wheel::string{ROUTE_ERROR_STRINGS[static_cast<uint8_t>(kind)]} + ": " + message;
}

HttpParser::HttpParser() {}

HttpParser::~HttpParser() {}

namespace router_details {
  HttpRouteNode::HttpRouteNode() {}
  HttpRouteNode::~HttpRouteNode() {}

  AddRouteResult HttpRouteNode::add_route(HttpMethod method, wheel::string_view path,
                                          RouteHandler&& handler) {
    spdlog::debug("[HttpRouteNode::add_route] Adding route: {}", path);
    if (path.empty()) {
      spdlog::debug("[HttpRouteNode::add_route] for Method: {} Path: {}", method_cast(method),
                    path);
      auto& old = handlers[static_cast<uint8_t>(method)];
      spdlog::debug("[HttpRouteNode::add_route] old");
      if (old) {
        spdlog::error("[HttpRouteNode::add_route] Route already exists: {}", path);
        return AddRouteResult(AddRouteError(AddRouteErrorKind::Conflict, ""));
      }
      spdlog::debug("[HttpRouteNode::add_route] Route added: {}", path);
      old = wheel::make_shared<RouteHandler>(wheel::move(handler));
      return AddRouteResult({});
    }
    if (path[0] != '/') {
      return AddRouteResult(AddRouteError(AddRouteErrorKind::InvalidPath, ""));
    }
    auto pos = path.find('/', 1);
    auto sub = path.substr(1, pos);
    auto res = children.try_emplace(sub, wheel::make_shared<HttpRouteNode>());
    return res.first->second->add_route(method, path.substr(sub.length() + 1),
                                        wheel::move(handler));
  }

  RouteResult HttpRouteNode::route(Context& ctx) {
    spdlog::debug("[HttpRouteNode::route] Routing request: {}", ctx.current_path);
    if (ctx.current_path.empty()) {
      auto& handler = handlers[static_cast<uint8_t>(ctx.request.method)];
      if (handler == nullptr) {
        return RouteResult(RouteError(RouteErrorKind::Unimplemented, ""));
      }
      return (*handler)(ctx);
    }
    if (ctx.current_path[0] != '/') {
      return RouteResult(RouteError(RouteErrorKind::NotFound, ""));
    }
    auto pos = ctx.current_path.find('/', 1);
    auto sub = ctx.current_path.substr(1, pos);
    auto iter = this->children.find(sub);
    ctx.current_path = ctx.current_path.substr(sub.length() + 1);
    if (iter != this->children.end()) {
      auto res = iter->second->route(ctx);
      if (res.is_ok())
        return res;
      else if (res.is_err() && res.as_ref().unwrap_err().kind != RouteErrorKind::NotFound) {
        return res;
      }
    }
    iter = this->children.find("");
    if (iter != this->children.end()) {
      return iter->second->route(ctx);
    }
    return RouteResult(RouteError(RouteErrorKind::NotFound, ""));
  }
}  // namespace router_details

DefaultRouter::DefaultRouter() : root() {}

DefaultRouter::~DefaultRouter() {}

AddRouteResult DefaultRouter::add_route(HttpMethod method, wheel::string_view path,
                                        RouteHandler&& handler) {
  spdlog::debug("[HttpRouter::add_route] Adding route: {}", path);
  return root.add_route(method, path, wheel::move(handler));
}

RouteResult DefaultRouter::route(Context& ctx) {
  spdlog::debug("[HttpRouter::route] Routing request: {}", ctx.current_path);
  if (ctx.request.method == HttpMethod::UNKNOWN) {
    return RouteResult(RouteError(RouteErrorKind::Unknown, ""));
  }
  return root.route(ctx);
}

void DefaultRouter::error_handler(RouteError error, RouteHandler&& handler) {
  spdlog::debug("[HttpRouter::error_handler] Handling error: {}", error.to_string());
  if (error.kind == RouteErrorKind::NotFound) {
    this->error_handlers[static_cast<uint8_t>(error.kind)]
        = wheel::make_shared<RouteHandler>(wheel::move(handler));
  } else if (error.kind == RouteErrorKind::Unimplemented) {
    this->error_handlers[static_cast<uint8_t>(error.kind)]
        = wheel::make_shared<RouteHandler>(wheel::move(handler));
  } else {
    this->error_handlers[0] = wheel::make_shared<RouteHandler>(wheel::move(handler));
  }
}

HTTP_NAMESPACE_END
