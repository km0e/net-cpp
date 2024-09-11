/**
 * @file service.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Service class for HTTP server
 * @version 0.11
 * @date 2024-08-16
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_NET_HTTP_SERVICE
#  define XSL_NET_HTTP_SERVICE
#  include "xsl/coro.h"
#  include "xsl/logctl.h"
#  include "xsl/net/http/component/redirect.h"
#  include "xsl/net/http/component/static.h"
#  include "xsl/net/http/context.h"
#  include "xsl/net/http/def.h"
#  include "xsl/net/http/msg.h"
#  include "xsl/net/http/router.h"

#  include <memory>
XSL_HTTP_NB
using namespace xsl::io;

namespace impl_service {
  template <ABILike ABI, ABOLike ABO, RouterLike<std::size_t> R>
  struct InnerDetails {
    using abi_traits_type = AIOTraits<ABI>;
    using abo_traits_type = AIOTraits<ABO>;
    using in_dev_type = typename abi_traits_type::in_dev_type;
    using out_dev_type = typename abo_traits_type::out_dev_type;

    constexpr InnerDetails() : router{}, handlers{}, status_handlers{} {}
    R router;
    std::unordered_map<std::size_t, Handler<in_dev_type, out_dev_type>> handlers;
    std::unordered_map<Status, Handler<in_dev_type, out_dev_type>> status_handlers;
  };

  template <ABILike ABI, ABOLike ABO, RouterLike<std::size_t> R>
  class Service {
  public:
    //<async byte reader
    using in_dev_type = ABI;

    //<async byte writer
    using out_dev_type = ABO;

    using router_type = R;

    using context_type = HandleContext<in_dev_type, out_dev_type>;

    using handler_type = Handler<in_dev_type, out_dev_type>;

    using details_type = InnerDetails<in_dev_type, out_dev_type, router_type>;

    constexpr Service(std::unique_ptr<details_type>&& details) : details(std::move(details)) {}

    Task<Response<out_dev_type>> operator()(Request<in_dev_type>&& request) {
      INFO("New request: {} {}", request.view.method, request.view.path);
      auto route_ctx = RouteContext{request.method, request.view.path};

      auto route_res = this->details->router.route(route_ctx);
      auto ctx = context_type{route_ctx.current_path, std::move(request)};
      handler_type* handler = nullptr;
      if (!route_res) {
        auto iter = this->details->status_handlers.find(route_res.error());
        if (iter != this->details->status_handlers.end()) {
          co_await iter->second(ctx);
        } else {
          ctx.easy_resp(route_res.error());
        }
      } else {
        handler = &this->details->handlers.at(**route_res);
        auto status = co_await (*handler)(ctx);
        if (status) {
          auto iter = this->details->status_handlers.find(*status);
          if (iter != this->details->status_handlers.end()) {
            co_await iter->second(ctx);
          } else {
            ctx.easy_resp(*status);
          }
        }
      }
      co_return std::move(ctx).checkout();
    }

  private:
    std::unique_ptr<details_type> details;
  };
}  // namespace impl_service
/**
 * @brief ServiceBuilder
 *
 * @tparam LowerLayer the lower layer type, such as Tcp<Ip<4>>
 * @tparam R the router type, such as Router
 */
template <ABILike ABI, ABOLike ABO, RouterLike<std::size_t> R = Router>
class Service {
public:
  using abi_traits_type = AIOTraits<ABI>;
  using abo_traits_type = AIOTraits<ABO>;
  using in_dev_type = typename abi_traits_type::in_dev_type;
  using out_dev_type = typename abo_traits_type::out_dev_type;
  using handler_type = Handler<in_dev_type, out_dev_type>;
  using router_type = R;
  using details_type = impl_service::InnerDetails<in_dev_type, out_dev_type, router_type>;

  constexpr Service() : tag(1), details{std::make_unique<details_type>()} {}
  constexpr Service(Service&&) = default;
  constexpr Service& operator=(Service&&) = default;
  constexpr ~Service() {}

  /**
   * @brief Add a static file(s) handler
   *
   * @param path the path to handle
   * @param prefix the prefix of the static file(s)
   */
  constexpr void add_static(std::string_view path, StaticFileConfig&& cfg) {
    auto static_handler = create_static_handler<in_dev_type, out_dev_type>(std::move(cfg));
    this->add_route(Method::GET, path, std::move(static_handler));
  }

  constexpr void add_route(Method method, std::string_view path, handler_type&& handler) {
    LOG4("Adding route: {}", path);
    auto tag = this->tag++;
    this->details->handlers.try_emplace(tag, std::move(handler));
    this->details->router.add_route(method, path, std::move(tag));
  }

  constexpr void add_fallback(Method method, std::string_view path, handler_type&& handler) {
    LOG4("Adding fallback: {}", path);
    auto tag = this->tag++;
    this->details->handlers.try_emplace(tag, std::move(handler));
    this->details->router.add_fallback(method, path, std::move(tag));
  }
  /**
   * @brief Redirect
   *
   * @param method the method
   * @param path the path
   * @param target the target
   * @return void
   */
  constexpr void redirect(Method method, std::string_view path, std::string_view target) {
    LOG4("Redirecting: {} -> {}", path, target);
    auto tag = this->tag++;
    this->details->handlers.try_emplace(tag,
                                        create_redirect_handler<in_dev_type, out_dev_type>(target));
    this->details->router.add_fallback(method, path, std::move(tag));
  }

  constexpr void set_status_handler(Status kind, handler_type&& handler) {
    this->details->status_handlers.try_emplace(kind, std::move(handler));
  }
  /**
   * @brief Build the service
   *
   * @return impl_service::Service<in_dev_type, out_dev_type, router_type>
   */
  constexpr impl_service::Service<in_dev_type, out_dev_type, router_type> build(
      this Service&& self) {
    return {std::move(self.details)};
  }
  /**
   * @brief Build the service as shared pointer
   *
   * @return decltype(auto)
   */
  constexpr decltype(auto) build_shared(this Service&& self) {
    return std::make_shared<impl_service::Service<in_dev_type, out_dev_type, router_type>>(
        std::move(self.details));
  }

private:
  std::size_t tag;
  std::unique_ptr<details_type> details;
};
template <ABIOLike ABIO, RouterLike<std::size_t> R = Router>
constexpr Service<typename ABIO::template rebind<In>, typename ABIO::template rebind<Out>, R>
make_service() {
  return {};
}
template <RouterLike<std::size_t> R = Router>
constexpr Service<ABR, ABW> make_service() {
  return {};
}

XSL_HTTP_NE
#endif
