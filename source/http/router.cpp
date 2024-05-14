#include "xsl/http/context.h"
#include "xsl/http/msg.h"
#include "xsl/http/parse.h"
#include "xsl/http/router.h"
#include "xsl/wheel/wheel.h"

#include <spdlog/spdlog.h>

#include <cstdint>

HTTP_NAMESPACE_BEGIN
RouteHandleError::RouteHandleError() : message("") {}
RouteHandleError::RouteHandleError(wheel::string message) : message(message) {}
RouteHandleError::~RouteHandleError() {}
wheel::string RouteHandleError::to_string() const { return message; }

AddRouteError::AddRouteError(AddRouteErrorKind kind)
    : kind(kind), message(ADD_ROUTE_ERROR_STRINGS[static_cast<uint8_t>(kind)]) {}
AddRouteError::AddRouteError(AddRouteErrorKind kind, wheel::string message)
    : kind(kind), message(message) {}
AddRouteError::~AddRouteError() {}
std::string AddRouteError::to_string() const {
  return wheel::string{ADD_ROUTE_ERROR_STRINGS[static_cast<uint8_t>(kind)]} + ": " + message;
}

RouteError::RouteError() : kind(RouteErrorKind::Unknown), message("") {}
RouteError::RouteError(RouteErrorKind kind)
    : kind(kind), message(ROUTE_ERROR_STRINGS[static_cast<uint8_t>(kind)]) {}
RouteError::RouteError(RouteErrorKind kind, wheel::string message) : kind(kind), message(message) {}
RouteError::~RouteError() {}
wheel::string RouteError::to_string() const {
  return wheel::string{ROUTE_ERROR_STRINGS[static_cast<uint8_t>(kind)]} + ": " + message;
}

HttpParser::HttpParser() : view() {}

HttpParser::~HttpParser() {}

namespace router_details {
  HttpRouteNode::HttpRouteNode() : handlers(), children() {}
  HttpRouteNode::~HttpRouteNode() {}

  AddRouteResult HttpRouteNode::add_route(HttpMethod method, wheel::string_view path,
                                          RouteHandler&& handler) {
    SPDLOG_DEBUG("Adding route: {}", path);
    if (path.empty()) {
      SPDLOG_DEBUG("for Method: {} Path: {}", method_cast(method), path);
      auto& old = handlers[static_cast<uint8_t>(method)];
      SPDLOG_DEBUG("old");
      if (old) {
        SPDLOG_ERROR("Route already exists: {}", path);
        return AddRouteResult(AddRouteError(AddRouteErrorKind::Conflict, ""));
      }
      SPDLOG_DEBUG("Route added: {}", path);
      old = wheel::make_unique<RouteHandler>(wheel::move(handler));
      return AddRouteResult({});
    }
    if (path[0] != '/') {
      return AddRouteResult(AddRouteError(AddRouteErrorKind::InvalidPath, ""));
    }
    auto pos = path.find('/', 1);
    auto sub = path.substr(1, pos);
    auto res = children.lock()->try_emplace(sub, wheel::make_shared<HttpRouteNode>());
    return res.first->second->add_route(method, path.substr(sub.length() + 1),
                                        wheel::move(handler));
  }

  RouteResult HttpRouteNode::route(Context& ctx) {
    SPDLOG_TRACE("Routing path: {}", ctx.current_path);
    if (ctx.is_ok) {
      SPDLOG_DEBUG("Request already routed");
      auto& handler = handlers[static_cast<uint8_t>(ctx.request.method)];
      if (handler == nullptr) {
        return RouteResult(RouteError(RouteErrorKind::Unimplemented, ""));
      }
      RouteHandleResult res = (*handler)(ctx);
      if (res.is_ok()) {
        return RouteResult(res.unwrap());
      }
      return RouteResult(RouteError(RouteErrorKind::Unknown, ""));
    }
    if (ctx.current_path[0] != '/') {
      return RouteResult(RouteError(RouteErrorKind::NotFound, ""));
    }
    auto pos = ctx.current_path.find('/', 1);
    auto sub = ctx.current_path.substr(1, pos);
    // TODO: find and check is not thread safe
    auto iter = this->children.share()->find(sub);
    if (iter != this->children.share()->end()) {
      SPDLOG_DEBUG("Routing to child: {}", sub);
      auto current_path = ctx.current_path;
      ctx.current_path = ctx.current_path.substr(sub.length() + 1);
      if (ctx.current_path.empty()) {
        SPDLOG_DEBUG("This is the last child");
        ctx.is_ok = true;
      }
      auto res = iter->second->route(ctx);
      if (res.is_ok())
        return res;
      else if (res.is_err() && res.as_ref().unwrap_err().kind != RouteErrorKind::NotFound) {
        return res;
      }
      ctx.current_path = current_path;
      ctx.is_ok = false;
    }
    SPDLOG_DEBUG("Routing to default child");
    iter = this->children.share()->find("");
    if (iter != this->children.share()->end()) {
      ctx.is_ok = true;
      return iter->second->route(ctx);
    }
    return RouteResult(RouteError(RouteErrorKind::NotFound, ""));
  }
}  // namespace router_details

DefaultRouter::DefaultRouter() : root() {}

DefaultRouter::~DefaultRouter() {}

AddRouteResult DefaultRouter::add_route(HttpMethod method, wheel::string_view path,
                                        RouteHandler&& handler) {
  SPDLOG_DEBUG("Adding route: {}", path);
  return root.add_route(method, path, wheel::move(handler));
}

RouteResult DefaultRouter::route(Context& ctx) {
  SPDLOG_DEBUG("Starting routing path: {}", ctx.current_path);
  if (ctx.request.method == HttpMethod::UNKNOWN) {
    return RouteResult(RouteError(RouteErrorKind::Unknown, ""));
  }
  return root.route(ctx);
}

void DefaultRouter::error_handler(RouteErrorKind kind, RouteHandler&& handler) {
  if (kind == RouteErrorKind::NotFound) {
    this->error_handlers[static_cast<uint8_t>(kind)]
        = wheel::make_shared<RouteHandler>(wheel::move(handler));
  } else if (kind == RouteErrorKind::Unimplemented) {
    this->error_handlers[static_cast<uint8_t>(kind)]
        = wheel::make_shared<RouteHandler>(wheel::move(handler));
  } else {
    this->error_handlers[0] = wheel::make_shared<RouteHandler>(wheel::move(handler));
  }
}

HTTP_NAMESPACE_END
