#include "xsl/logctl.h"
#include "xsl/net/http/msg.h"
#include "xsl/net/http/proto.h"
#include "xsl/net/http/router.h"

HTTP_NB
std::string_view to_string_view(RouteError re) {
  return ROUTE_ERROR_STRINGS[static_cast<uint8_t>(re)];
}

RouteContext::RouteContext(Request&& request)
    : current_path(request.view.url), request(std::move(request)) {}
RouteContext::~RouteContext() {}

AddRouteError::AddRouteError(AddRouteErrorKind kind)
    : kind(kind), message(ADD_ROUTE_ERROR_STRINGS[static_cast<uint8_t>(kind)]) {}
AddRouteError::AddRouteError(AddRouteErrorKind kind, std::string message)
    : kind(kind), message(message) {}
AddRouteError::~AddRouteError() {}
std::string AddRouteError::to_string() const {
  return std::string{ADD_ROUTE_ERROR_STRINGS[static_cast<uint8_t>(kind)]} + ": " + message;
}

// RouteError::RouteError() : kind(RouteErrorKind::Unknown), message("") {}
// RouteError::RouteError(RouteErrorKind kind)
//     : kind(kind), message(ROUTE_ERROR_STRINGS[static_cast<uint8_t>(kind)]) {}
// RouteError::RouteError(RouteErrorKind kind, std::string message) : kind(kind), message(message)
// {} RouteError::~RouteError() {} std::string RouteError::to_string() const {
//   return std::string{ROUTE_ERROR_STRINGS[static_cast<uint8_t>(kind)]} + ": " + message;
// }


namespace router_details {
  HttpRouteNode::HttpRouteNode() : handlers(), children() {}
  HttpRouteNode::HttpRouteNode(HttpMethod method, RouteHandler&& handler) : handlers(), children() {
    handlers[static_cast<uint8_t>(method)] = std::move(handler);
  }
  HttpRouteNode::~HttpRouteNode() {}

  AddRouteResult HttpRouteNode::add_route(HttpMethod method, std::string_view path,
                                          RouteHandler&& handler) {
    LOG5("Adding route: {}", path);
    if (path[0] != '/') {
      return std::unexpected(AddRouteError(AddRouteErrorKind::InvalidPath, ""));
    }
    auto pos = path.find('/', 1);

    if (pos != std::string_view::npos) {
      auto sub = path.substr(1, pos - 1);
      auto res = children.lock()->find(sub);
      if (res != children.lock()->end()) {
        return res->second.add_route(method, path.substr(sub.length() + 1), std::move(handler));
      }
      return children.lock()
          ->try_emplace(std::string{sub})
          .first->second.add_route(method, path.substr(sub.length() + 1), std::move(handler));
    }
    auto sub_path = path.substr(1);
    auto child = children.lock()->find(sub_path);

    if (child == children.lock()->end()) {
      children.lock()->try_emplace(std::string{sub_path}, method, std::move(handler));
      return AddRouteResult();
    }
    if (child->second.add(method, std::move(handler))) {
      return AddRouteResult();
    }
    return std::unexpected(AddRouteError(AddRouteErrorKind::Conflict, ""));
  }

  RouteResult HttpRouteNode::route(RouteContext& ctx) {
    LOG6("Routing path: {}", ctx.current_path);
    if (ctx.current_path[0] != '/') {
      return std::unexpected{RouteError::NotFound};
    }
    auto pos = ctx.current_path.find('/', 1);
    do {
      if (pos != std::string_view::npos) {  // if there is a sub path
        auto sub = ctx.current_path.substr(1, pos - 1);
        auto child = children.lock_shared()->find(sub);
        if (child != children.lock_shared()->end()) {  // if the sub path is found
          auto current_path = ctx.current_path;        // save current path
          ctx.current_path = ctx.current_path.substr(sub.length() + 1);
          auto res = child->second.route(ctx);
          if (res.has_value()) {  // if handled
            return res;
          }
          if ((!res.has_value())
              && res.error() != RouteError::NotFound) {  // if error but not found
            return res;
          }
          ctx.current_path = current_path;  // restore current path
        }
        break;
      }
      auto child = children.lock_shared()->find(ctx.current_path.substr(1));
      if (child == children.lock_shared()->end()) {  // if the path is not found
        break;
      }
      ctx.current_path = "";  // set current path to empty
      auto dr_res = child->second.direct_route(ctx);
      if (dr_res.has_value()) {
        return dr_res;
      }
      if ((!dr_res.has_value()) && dr_res.error() != RouteError::NotFound) {
        return dr_res;
      }
    } while (false);
    LOG5("Routing to default child");
    auto iter = this->children.lock_shared()->find("");  // find default child
    if (iter != this->children.lock_shared()->end()) {
      return iter->second.direct_route(ctx);
    }
    return std::unexpected{RouteError::NotFound};
  }
}  // namespace router_details

namespace router_details {
  bool HttpRouteNode::add(HttpMethod method, RouteHandler&& handler) {
    if (handlers[static_cast<uint8_t>(method)]) {
      return false;
    }
    handlers[static_cast<uint8_t>(method)] = std::move(handler);
    return true;
  }
  RouteResult HttpRouteNode::direct_route(RouteContext& ctx) {
    LOG5("Direct routing path: {}", ctx.current_path);
    auto& handler = handlers[static_cast<uint8_t>(ctx.request.method)];
    if (handler == nullptr) {
      return std::unexpected{RouteError::Unimplemented};
    }
    return &handler;
  }

}  // namespace router_details

HttpRouter::HttpRouter() : root() {}

HttpRouter::~HttpRouter() {}

AddRouteResult HttpRouter::add_route(HttpMethod method, std::string_view path,
                                     RouteHandler&& handler) {
  LOG4("Adding route: {}", path);
  return root.add_route(method, path, std::move(handler));
}

RouteResult HttpRouter::route(RouteContext& ctx) {
  LOG4("Starting routing path: {}", ctx.current_path);
  if (ctx.request.method == HttpMethod::UNKNOWN) {
    return std::unexpected{RouteError::Unknown};
  }
  return root.route(ctx);
}

void HttpRouter::set_error_handler(RouteError kind, RouteHandler&& handler) {
  if (kind == RouteError::NotFound) {
    this->error_handlers[static_cast<uint8_t>(kind)]
        = make_shared<RouteHandler>(std::move(handler));
  } else if (kind == RouteError::Unimplemented) {
    this->error_handlers[static_cast<uint8_t>(kind)]
        = make_shared<RouteHandler>(std::move(handler));
  } else {
    this->error_handlers[0] = make_shared<RouteHandler>(std::move(handler));
  }
}
const RouteHandler* HttpRouter::error_handle(RouteError kind) {
  return this->error_handlers[static_cast<uint8_t>(kind)].get();
}

HTTP_NE
