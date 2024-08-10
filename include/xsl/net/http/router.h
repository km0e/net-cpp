#pragma once
#ifndef XSL_NET_HTTP_ROUTER
#  define XSL_NET_HTTP_ROUTER
#  include "xsl/logctl.h"
#  include "xsl/net/http/def.h"
#  include "xsl/net/http/msg.h"
#  include "xsl/net/http/proto.h"
#  include "xsl/sync.h"
#  include "xsl/wheel.h"

#  include <cstddef>
#  include <expected>
#  include <string_view>

XSL_HTTP_NB

class RouteContext {
public:
  RouteContext(Method method, std::string_view current_path)
      : method(method), current_path(current_path) {}
  RouteContext(RouteContext&&) = default;
  RouteContext& operator=(RouteContext&&) = default;
  ~RouteContext() {}
  Method method;
  std::string_view current_path;
};

using RouteResult = std::expected<const std::size_t*, RouteError>;

template <class R, class Tag>
concept RouterLike = requires(R r, Method hm, std::string_view path, Tag&& tag, RouteContext& ctx) {
  { r.add_route(hm, path, std::move(tag)) } -> std::same_as<void>;
  { r.route(ctx) } -> std::same_as<RouteResult>;
};

namespace router_details {
  class HttpRouteNode {
  public:
    using tag_type = std::size_t;
    HttpRouteNode() : handlers(), children() {}
    HttpRouteNode(Method method, tag_type&& tag) : handlers(), children() {
      handlers[static_cast<uint8_t>(method)] = std::move(tag);
    }
    ~HttpRouteNode() {}

    void add_route(Method method, std::string_view path, tag_type&& tag) {
      LOG5("Adding route: {}", path);
      if (path[0] != '/') {
        throw std::invalid_argument("Invalid path");
      }
      auto pos = path.find('/', 1);

      if (pos != std::string_view::npos) {
        auto sub = path.substr(1, pos - 1);
        auto res = children.lock()->find(sub);
        if (res != children.lock()->end()) {
          return res->second.add_route(method, path.substr(sub.length() + 1), std::move(tag));
        }
        return children.lock()
            ->try_emplace(std::string{sub})
            .first->second.add_route(method, path.substr(sub.length() + 1), std::move(tag));
      }
      auto sub_path = path.substr(1);
      auto child = children.lock()->find(sub_path);

      if (child == children.lock()->end()) {
        children.lock()->try_emplace(std::string{sub_path}, method, std::move(tag));
        return;
      }
      if (child->second.add(method, std::move(tag))) {
        return;
      }
      throw std::invalid_argument("Route already exists");
    }

    RouteResult route(RouteContext& ctx) {
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

  private:
    std::array<std::size_t, HTTP_METHOD_COUNT> handlers;
    ShardRes<us_map<HttpRouteNode>> children;

    bool add(Method method, tag_type&& tag) {
      if (handlers[static_cast<uint8_t>(method)]) {
        return false;
      }
      handlers[static_cast<uint8_t>(method)] = std::move(tag);
      return true;
    }
    RouteResult direct_route(RouteContext& ctx) {
      LOG5("Direct routing path: {}", ctx.current_path);
      auto& handler = handlers[static_cast<uint8_t>(ctx.method)];
      if (handler == tag_type{}) {
        return std::unexpected{RouteError::Unimplemented};
      }
      return &handler;
    }
  };
}  // namespace router_details

class Router {
public:
  using tag_type = std::size_t;
  Router() : root() {}

  ~Router() {}
  void add_route(Method method, std::string_view path, tag_type&& tag) {
    LOG4("Adding route: {}", path);
    return root.add_route(method, path, std::move(tag));
  }

  RouteResult route(RouteContext& ctx) {
    LOG4("Starting routing path: {}", ctx.current_path);
    if (ctx.method == Method::UNKNOWN) {
      return std::unexpected{RouteError::Unknown};
    }
    return root.route(ctx);
  }

private:
  router_details::HttpRouteNode root;
};

static_assert(RouterLike<Router, std::size_t>, "Router is not a Router");

XSL_HTTP_NE
#endif
