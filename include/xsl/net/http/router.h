/**
 * @file router.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief HTTP router
 * @version 0.1
 * @date 2024-09-01
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_NET_HTTP_ROUTER
#  define XSL_NET_HTTP_ROUTER
#  include "xsl/logctl.h"
#  include "xsl/net/http/def.h"
#  include "xsl/net/http/proto.h"
#  include "xsl/wheel.h"

#  include <cassert>
#  include <memory>
#  include <string_view>

XSL_HTTP_NB

class RouteContext {
public:
  constexpr RouteContext(Method method, std::string_view current_path)
      : method(method), current_path(current_path) {}
  constexpr RouteContext(RouteContext&&) = default;
  constexpr RouteContext& operator=(RouteContext&&) = default;
  constexpr ~RouteContext() {}
  Method method;
  std::string_view current_path;
};

template <class R, class Tag>
concept RouterLike = requires(R r, std::string_view path, RouteContext& ctx) {
  { r.add_exact(path) } -> std::same_as<Tag&>;
  { r.add_prefix(path) } -> std::same_as<Tag&>;
  { r.route(ctx) } -> std::same_as<const Tag*>;
};

namespace router_details {
  template <class Id>
  class HttpRouteNode {
  public:
    constexpr HttpRouteNode() : handlers{}, prefix_handler{}, children{} {}
    constexpr ~HttpRouteNode() {}

    /**
     * @brief Add an exact route
     *
     * @param path, the path of the route
     * @return Id&, the handler for the route
     */
    Id& add_exact(std::string_view path) {
      rt_assert(path[0] == '/', "Invalid path");
      auto pos = path.find('/', 1);

      if (pos != std::string_view::npos) {
        auto sub = path.substr(1, pos - 1);
        auto res = children.find(sub);
        if (res != children.end()) {
          return res->second.add_exact(path.substr(pos));
        }
        return children.try_emplace(std::string{sub}).first->second.add_exact(path.substr(pos));
      }
      auto sub_path = path.substr(1);
      auto it = handlers.find(sub_path);

      if (it == handlers.end()) {
        return handlers.try_emplace(std::string{sub_path})
            .first->second;  ///< If the path is not found, create a new handler
      }
      return it->second;
    }

    /**
     * @brief Add a prefix route
     *
     * @param path, the path prefix
     * @return Id&, the handler for the prefix
     */
    constexpr Id& add_prefix(std::string_view path) {
      rt_assert(path[0] == '/', "Invalid path");
      if (path == "/") {
        return prefix_handler;  ///< If the path is empty, return the fallback handler
      }
      path = path.substr(1);  ///< Remove the leading slash
      auto pos = path.find('/');
      auto sub = path.substr(0, pos);
      auto res = children.find(sub);
      if (res == children.end()) {
        res = children.try_emplace(std::string{sub}).first;
      }
      if (pos == std::string_view::npos) {  // if there is no sub path
        return res->second.prefix_handler;  ///< If there is no sub path, return the prefix handler
      }
      return res->second.add_prefix(path.substr(pos));
    }
    /**
     * @brief Route the request
     *
     * @param ctx the route context
     * @return const Id*, the handler for the route, or nullptr if not found
     */
    constexpr const Id* route(RouteContext& ctx) {
      log_trace("Routing: {}", ctx.current_path);
      if (ctx.current_path[0] != '/') {
        return nullptr;  ///< If the path does not start with '/', return nullptr
      }
      auto pos = ctx.current_path.find('/', 1);
      do {
        if (pos != std::string_view::npos) {  // if there is a sub path
          auto sub = ctx.current_path.substr(1, pos - 1);
          auto child = children.find(sub);
          if (child != children.end()) {           // if the sub path is found
            auto current_path = ctx.current_path;  // save current path
            ctx.current_path = ctx.current_path.substr(pos);
            auto res = child->second.route(ctx);
            if (res) return res;
            ctx.current_path = current_path;  // restore current path
          }
          break;
        }
        // if there is no sub path, we need to handle the current path
        auto sub_path = ctx.current_path.substr(1);
        auto it = handlers.find(sub_path);
        if (it != handlers.end()) {  // if the path is found
          log_debug("Routing to exact handler: {}", sub_path);
          auto p = &it->second;  // return the handler
          return p;
        }
      } while (false);
      log_debug("Routing to prefix handler");
      if (!!prefix_handler) {
        log_debug("Routing to fallback handler");
        return &prefix_handler;  ///< If no exact match, return the fallback handler
      }
      return nullptr;  ///< If no match, return nullptr
    }

  private:
    us_map<Id> handlers;
    Id prefix_handler;
    us_map<HttpRouteNode> children;
  };
}  // namespace router_details

template <class Id>
class Router {
public:
  constexpr Router() : root(std::make_unique<router_details::HttpRouteNode<Id>>()) {}
  constexpr Router(Router&&) = default;

  constexpr decltype(auto) add_exact(std::string_view path) { return root->add_exact(path); }

  constexpr decltype(auto) add_prefix(std::string_view path) { return root->add_prefix(path); }

  constexpr const Id* route(RouteContext& ctx) { return root->route(ctx); }

private:
  std::unique_ptr<router_details::HttpRouteNode<Id>> root;
};

static_assert(RouterLike<Router<int>, int>, "Router is not a Router");

XSL_HTTP_NE
#endif
