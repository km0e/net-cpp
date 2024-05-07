#include <spdlog/spdlog.h>
#include <xsl/http/http.h>
#include <xsl/http/msg.h>
#include <xsl/http/router.h>
#include <xsl/http/server.h>
#include <xsl/utils/wheel/wheel.h>

#include <cstdint>

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
      auto& old = handlers[static_cast<uint8_t>(method)];
      if (old != nullptr) {
        return AddRouteResult(AddRouteError(AddRouteErrorKind::Conflict, ""));
      }
      old = wheel::make_shared<RouteHandler>(wheel::move(handler));
      return AddRouteResult({});
    }
    if (path[0] != '/') {
      return AddRouteResult(AddRouteError(AddRouteErrorKind::InvalidPath, ""));
    }
    auto pos = path.find('/', 1);
    auto sub = path.substr(1, pos);
    auto res = children.try_emplace(sub);
    return res.first->second->add_route(method, path.substr(sub.length() + 1),
                                        wheel::move(handler));
  }

  RouteResult HttpRouteNode::route(HttpRequest& request) {
    spdlog::debug("[HttpRouteNode::route] Routing request: {}", request.path);
    if (request.path.empty()) {
      auto& handler = handlers[static_cast<uint8_t>(request.method)];
      if (handler == nullptr) {
        return RouteResult(RouteError(RouteErrorKind::Unimplemented, ""));
      }
      return RouteResult(handler);
    }
    if (request.path[0] != '/') {
      return RouteResult(RouteError(RouteErrorKind::NotFound, ""));
    }
    auto pos = request.path.find('/', 1);
    auto sub = request.path.substr(1, pos);
    auto iter = this->children.find(sub);
    if (iter != this->children.end()) {
      auto res = iter->second->route(request);
      if (res.is_ok())
        return res;
      else if (res.is_err() && res.as_ref().unwrap_err().kind != RouteErrorKind::NotFound) {
        return res;
      }
    }
    iter = this->children.find("");
    if (iter != this->children.end()) {
      return iter->second->route(request);
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

RouteResult DefaultRouter::route(HttpRequest& request) {
  spdlog::debug("[HttpRouter::route] Routing request: {}", request.path);
  if (request.method == HttpMethod::UNKNOWN) {
    return RouteResult(RouteError(RouteErrorKind::Unknown, ""));
  }
  auto res = root.route(request);
  return root.route(request);
}
void DefaultRouter::error_handler(RouteError error, RouteHandler&& handler) {
  spdlog::debug("[HttpRouter::error_handler] Handling error: {}", error.to_string());
  if (error.kind == RouteErrorKind::NotFound) {
    this->error_handlers[static_cast<uint8_t>(error.kind)] = wheel::make_shared<RouteHandler>(wheel::move(handler));
  } else if (error.kind == RouteErrorKind::Unimplemented) {
    this->error_handlers[static_cast<uint8_t>(error.kind)] = wheel::make_shared<RouteHandler>(wheel::move(handler));
  } else {
    this->error_handlers[0] = wheel::make_shared<RouteHandler>(wheel::move(handler));
  }
}

HTTP_NAMESPACE_END
