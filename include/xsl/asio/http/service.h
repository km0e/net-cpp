/**
 * @file service.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Service class for HTTP server
 * @version 0.2.0
 * @date 2024-08-16
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once

#ifndef XSL_ASIO_HTTP_SERVICE
#  define XSL_ASIO_HTTP_SERVICE
#  include <xsl/asio/http/component/redirect.h>
#  include <xsl/asio/http/component/static.h>
#  include <xsl/asio/http/context.h>
#  include <xsl/asio/http/def.h>
#  include <xsl/coro.h>
#  include <xsl/log.h>
#  include <xsl/net.h>
#  include <xsl/wheel.h>

#  include <utility>
XSL_ASIO_HTTP_NB
using namespace xsl::io;
using namespace xsl::http;

namespace _impl_service {
  using namespace xsl::http;
  struct Id {
    std::size_t value;
    constexpr Id() : value(0) {}  ///< Default constructor, initializes to 0
    constexpr Id(std::size_t v) : value(v) {}
    constexpr bool operator!() const { return this->value == 0; }  ///< Check if id is not set
    constexpr bool operator==(const Id& other) const { return this->value == other.value; }
  };
}  // namespace _impl_service

template <AsyncRead I, AsyncWrite O, RouterLike<_impl_service::Id> R>
struct Service {
  using in_dev_type = I;
  using out_dev_type = O;
  using id_type = _impl_service::Id;
  using handler_type = Handler<I, O>;
  using context_type = HandleContext<I, O>;

  constexpr Service() : router{}, handlers{}, status_handlers{} {}
  R router;
  std::unordered_map<std::pair<id_type, Method>, handler_type> handlers;
  std::unordered_map<Status, handler_type> status_handlers;

  /**
   * @brief Operator to handle a request
   *
   * @param request
   * @param in_dev
   */
  Task<ResponseBuilder<O>> operator()(Request& request, I& in_dev) {
    log_info("New request: {} {}", request.line.method, request.line.path);
    auto route_ctx = RouteContext{request.line.method, request.line.path};

    const id_type* hid = this->router.route(route_ctx);
    auto ctx = context_type(route_ctx.current_path, request, in_dev);
    if (!hid) {
      ctx.easy_resp(Status::NOT_FOUND);
    } else {
      handler_type* handler = nullptr;
      if (auto iter = handlers.find({*hid, request.line.method}); iter != handlers.end()) {
        handler = &iter->second;
      } else if (auto iter = handlers.find({*hid, Method::UNKNOWN}); iter != handlers.end()) {
        handler = &iter->second;
      } else {
        rt_assert(false, std::format("No handler found for id: {}, method: {}", hid->value,
                                     request.line.method.to_string_view()));
      }
      auto status = co_await (*handler)(ctx);
      if (status) {
        auto iter = this->status_handlers.find(*status);
        if (iter != this->status_handlers.end()) {
          co_await iter->second(ctx);
        } else {
          ctx.easy_resp(*status);
        }
      }
    }
    co_return std::move(ctx).checkout();
  }
};

/**
 * @brief ServiceBuilder class to build a service with handlers and static files
 */
template <AsyncRead I, AsyncWrite O, RouterLike<_impl_service::Id> Rt>
class ServiceBuilder {
public:
  using handler_type = Handler<I, O>;
  using id_type = _impl_service::Id;

  constexpr ServiceBuilder() = default;
  constexpr ServiceBuilder(ServiceBuilder&&) = default;
  constexpr ServiceBuilder& operator=(ServiceBuilder&&) = default;
  constexpr ~ServiceBuilder() {}

  /**
   * @brief Add a static file handler
   *
   * @param path, the path to the static file
   * @param cfg, the configuration for the static file handler
   */
  constexpr void add_static(std::string_view path, StaticFileConfig&& cfg) {
    auto static_handler = create_static_handler<I, O>(std::move(cfg));
    this->add_prefix(path, std::move(static_handler));
  }

  /**
   * @brief Add a route handler
   *
   * @param method, the HTTP method
   * @param path, the path for the route
   * @param handler, the handler for the route
   */
  constexpr void add_route(Method method, std::string_view path, handler_type&& handler) {
    log_info("Adding route: {} with method: {}", path, method.to_string_view());
    this->add_handler(s.router.add_exact(path), std::move(handler), method);
  }

  /**
   * @brief Add a prefix handler
   *
   * @param path, the path prefix
   * @param handler, the handler for the prefix
   */
  constexpr void add_prefix(std::string_view path, handler_type&& handler) {
    log_info("Adding prefix: {}", path);
    this->add_handler(s.router.add_prefix(path), std::move(handler), Method::UNKNOWN);
  }

  /**
   * @brief Redirect
   *
   * @param method, the method
   * @param path, the path
   * @param target, the target
   * @return void
   */
  constexpr void redirect(Method method, std::string_view path, std::string_view target) {
    log_info("Redirecting: {} -> {}", path, target);
    add_handler(s.router.add_exact(path), create_redirect_handler<I, O>(target), method);
  }

  constexpr void set_status_handler(Status kind, handler_type&& handler) {
    log_info("Setting status handler for status: {}", kind.to_string_view());
    this->s.status_handlers.try_emplace(kind, std::move(handler));
  }
  /**
   * @brief Build the service
   *
   * @return Service<in_dev_type, out_dev_type, router_type>
   */
  constexpr auto build(this ServiceBuilder&& self) { return std::move(self.s); }

private:
  Service<I, O, Rt> s = {};  ///< Details of the service, including handlers and status handlers
  id_type _id = 1;           ///< Id for path handlers
  constexpr void try_update_id(id_type& id) {
    if (!id) {
      id = std::exchange(_id,
                         _id.value + 1);  ///< Try to update the id if it is not set
    }
  }

  constexpr void add_handler(id_type& id, handler_type&& handler, Method method) {
    this->try_update_id(id);  ///< Try to update the id if it is not set
    this->s.handlers.emplace(std::make_pair(id, method), std::move(handler));
  }
};

XSL_ASIO_HTTP_NE
namespace std {
  template <>
  struct hash<std::pair<xsl::_asio::http::_impl_service::Id, Method>> {
    std::size_t operator()(
        const std::pair<xsl::_asio::http::_impl_service::Id, Method>& p) const noexcept {
      return std::hash<std::size_t>()(p.first.value)
             ^ std::hash<decltype(Method::_method)>()(p.second._method);
    }
  };
}  // namespace std
#endif
