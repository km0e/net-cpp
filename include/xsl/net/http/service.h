/**
 * @file service.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-08-16
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_NET_HTTP_SERVICE
#  define XSL_NET_HTTP_SERVICE
#  include "xsl/ai.h"
#  include "xsl/coro.h"
#  include "xsl/net/http/component/redirect.h"
#  include "xsl/net/http/component/static.h"
#  include "xsl/net/http/context.h"
#  include "xsl/net/http/def.h"
#  include "xsl/net/http/msg.h"
#  include "xsl/net/http/router.h"

#  include <memory>
XSL_HTTP_NB

namespace impl_service {
  template <ai::ABRL In, ai::ABWL Out, RouterLike<std::size_t> R>
  struct InnerDetails {
    InnerDetails() : router{}, handlers{}, status_handlers{} {}
    R router;
    std::unordered_map<std::size_t, Handler<In, Out>> handlers;
    std::unordered_map<Status, Handler<In, Out>> status_handlers;
  };

  template <ai::ABRL ABr, ai::ABWL ABw, RouterLike<std::size_t> R>
  class Service {
  public:
    //<async byte reader
    using abr_type = ABr;

    //<async byte writer
    using abw_type = ABw;

    using router_type = R;

    using context_type = HandleContext<abr_type, abw_type>;

    using handler_type = Handler<abr_type, abw_type>;

    using details_type = InnerDetails<abr_type, abw_type, router_type>;

    Service(std::unique_ptr<details_type>&& details) : details(std::move(details)) {}

    Task<Response<abw_type>> operator()(Request<abr_type>&& request) {
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
 * @tparam LowerLayer the lower layer type, such as feature::Tcp<feature::Ip<4>>
 * @tparam R the router type, such as Router
 */
template <ai::ABRL ABr, ai::ABWL ABw, RouterLike<std::size_t> R = Router>
class Service {
public:
  using abr_type = ABr;
  using abw_type = ABw;
  using handler_type = Handler<abr_type, abw_type>;
  using router_type = R;
  using details_type = impl_service::InnerDetails<abr_type, abw_type, router_type>;

  Service() : tag(1), details{std::make_unique<details_type>()} {}
  Service(Service&&) = default;
  Service& operator=(Service&&) = default;
  ~Service() {}

  /**
   * @brief Add a static file(s) handler
   *
   * @param path the path to handle
   * @param prefix the prefix of the static file(s)
   */
  void add_static(std::string_view path, StaticFileConfig&& cfg) {
    auto static_handler = create_static_handler<abr_type, abw_type>(std::move(cfg));
    this->add_route(Method::GET, path, std::move(static_handler));
  }

  void add_route(Method method, std::string_view path, handler_type&& handler) {
    LOG4("Adding route: {}", path);
    auto tag = this->tag++;
    this->details->handlers.try_emplace(tag, std::move(handler));
    this->details->router.add_route(method, path, std::move(tag));
  }

  void add_fallback(Method method, std::string_view path, handler_type&& handler) {
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
  void redirect(Method method, std::string_view path, std::string_view target) {
    LOG4("Redirecting: {} -> {}", path, target);
    auto tag = this->tag++;
    this->details->handlers.try_emplace(tag, create_redirect_handler<abr_type, abw_type>(target));
    this->details->router.add_fallback(method, path, std::move(tag));
  }

  void set_status_handler(Status kind, handler_type&& handler) {
    this->details->status_handlers.try_emplace(kind, std::move(handler));
  }
  /**
   * @brief Build the service
   *
   * @return impl_service::Service<abr_type, abw_type, router_type>
   */
  impl_service::Service<abr_type, abw_type, router_type> build(this Service self) {
    return {std::move(self.details)};
  }
  /**
   * @brief Build the service as shared pointer
   *
   * @return decltype(auto)
   */
  decltype(auto) build_shared(this Service self) {
    return std::make_shared<impl_service::Service<abr_type, abw_type, router_type>>(
        std::move(self.details));
  }

private:
  std::size_t tag;
  std::unique_ptr<details_type> details;
};
template <ai::ABRWL ABrw, RouterLike<std::size_t> R = Router>
Service<typename ABrw::in_dev_type, typename ABrw::out_dev_type, R> make_service() {
  return {};
}
template <RouterLike<std::size_t> R = Router>
Service<ABR, ABW> make_service() {
  return {};
}

XSL_HTTP_NE
#endif
