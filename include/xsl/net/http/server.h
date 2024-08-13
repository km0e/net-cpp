#pragma once
#ifndef XSL_NET_HTTP_SERVER
#  define XSL_NET_HTTP_SERVER
#  include "xsl/ai/dev.h"
#  include "xsl/coro.h"
#  include "xsl/feature.h"
#  include "xsl/logctl.h"
#  include "xsl/net/http/component.h"
#  include "xsl/net/http/context.h"
#  include "xsl/net/http/def.h"
#  include "xsl/net/http/msg.h"
#  include "xsl/net/http/parse.h"
#  include "xsl/net/http/proto.h"
#  include "xsl/net/http/router.h"
#  include "xsl/net/tcp.h"

#  include <cstddef>
#  include <expected>
#  include <memory>
#  include <string_view>
#  include <unordered_map>
#  include <utility>

XSL_HTTP_NB
namespace impl_server {

  template <RouterLike<std::size_t> R, ai::AsyncReadDeviceLike<std::byte> In,
            ai::AsyncWriteDeviceLike<std::byte> Out>
  struct InnerDetails {
    InnerDetails() : router(), handlers(), status_handlers() {}
    R router;
    std::unordered_map<std::size_t, Handler<In, Out>> handlers;
    std::unordered_map<Status, Handler<In, Out>> status_handlers;
  };

  /**
   * @brief HttpServer
   *
   * @tparam Server the lower server type, such as TcpServer
   * @tparam R the router type, such as Router
   */
  template <class LowerServer, RouterLike<std::size_t> R = Router>
  class Server {
  public:
    using lower_type = LowerServer;
    using io_dev_type = typename lower_type::io_dev_type;
    using in_dev_type = typename lower_type::in_dev_type;
    using out_dev_type = typename lower_type::out_dev_type;
    using context_type = HandleContext<in_dev_type, out_dev_type>;
    using handler_type = Handler<in_dev_type, out_dev_type>;
    using details_type = InnerDetails<R, in_dev_type, out_dev_type>;

    Server(lower_type&& server, std::unique_ptr<details_type>&& details)
        : server(std::move(server)), details(std::move(details)) {}

    Server(Server&&) = default;
    Server& operator=(Server&&) = default;
    ~Server() {}

    /**
     * @brief Run the server
     *
     * @tparam Executor the executor type, default is coro::ExecutorBase
     * @return coro::Task<std::expected<void, std::errc>, Executor>
     */
    template <class Executor = coro::ExecutorBase>
    coro::Lazy<std::expected<void, std::errc>, Executor> run() {
      LOG4("HttpServer start at: {}:{}", this->server.host, this->server.port);
      while (true) {
        auto res = co_await this->server.template accept<Executor>(nullptr);
        if (!res) {
          LOG2("accept error: {}", std::make_error_code(res.error()).message());
          continue;
        }
        http_connection<Executor>(std::move(*res)).detach(co_await coro::GetExecutor<Executor>());
      }
    }

  private:
    lower_type server;
    std::unique_ptr<details_type> details;

    template <class Executor = coro::ExecutorBase>
    coro::Lazy<void, Executor> http_connection(io_dev_type dev) {
      auto [ard, awd] = std::move(dev).split();
      auto parser = Parser<HttpParseTrait>{};
      ParseData parse_data{};
      while (true) {
        {
          auto res = co_await parser.template read<Executor>(ard, parse_data);
          if (!res) {
            if (res.error() != std::errc::no_message) {
              LOG3("recv error: {}", std::make_error_code(res.error()).message());
            }
            break;
          }
          LOG4("New request: {} {}", parse_data.request.method, parse_data.request.path);
        }

        bool keep_alive = true;
        if (auto iter = parse_data.request.headers.find("Connection");
            iter == parse_data.request.headers.end() || iter->second != "keep-alive") {
          keep_alive = false;
        }

        Request<in_dev_type> request{std::move(parse_data.buffer), std::move(parse_data.request),
                                     parse_data.content_part, ard};

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
        auto [sz, err] = co_await ctx.sendto(awd);
        if (err) {
          LOG3("send error: {}", std::make_error_code(*err).message());
        }
        if (!keep_alive) {
          break;
        }
      }
    }
  };
}  // namespace impl_server

/**
 * @brief ServerBuilder
 *
 * @tparam LowerLayer the lower layer type, such as feature::Tcp<feature::Ip<4>>
 * @tparam R the router type, such as Router
 */
template <class LowerLayer, RouterLike<std::size_t> R = Router>
class ServerBuilder {
public:
  template <class... Flags>
  struct _CreateFeature;

  template <class _LowerLayer>
  struct _CreateFeature<feature::Tcp<_LowerLayer>> {
    using type = _net::TcpServer<_LowerLayer>;
  };

  template <class _LowerLayer>
  struct _CreateFeature<_LowerLayer> {
    static_assert(false, "unsupported feature");
  };

  using server_type = _CreateFeature<LowerLayer>::type;
  using io_dev_type = typename server_type::io_dev_type;
  using in_dev_type = typename server_type::in_dev_type;
  using out_dev_type = typename server_type::out_dev_type;
  using context_type = HandleContext<in_dev_type, out_dev_type>;
  using handler_type = Handler<in_dev_type, out_dev_type>;
  using router_type = R;
  using details_type = impl_server::InnerDetails<R, in_dev_type, out_dev_type>;

  ServerBuilder() : tag(1), details{std::make_unique<details_type>()} {}
  ServerBuilder(router_type&& router)
      : tag(1), details{std::make_unique<details_type>(std::move(router))} {}

  ServerBuilder(ServerBuilder&&) = default;
  ServerBuilder& operator=(ServerBuilder&&) = default;
  ~ServerBuilder() {}

  /**
   * @brief Add a static file(s) handler
   *
   * @param path the path to handle
   * @param prefix the prefix of the static file(s)
   */
  void add_static(std::string_view path, StaticFileConfig&& cfg) {
    auto static_handler = create_static_handler<in_dev_type, out_dev_type>(std::move(cfg));
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
    this->details->handlers.try_emplace(tag,
                                        create_redirect_handler<in_dev_type, out_dev_type>(target));
    this->details->router.add_fallback(method, path, std::move(tag));
  }

  void set_status_handler(Status kind, handler_type&& handler) {
    this->details->status_handlers.try_emplace(kind, std::move(handler));
  }
  /**
   * @brief Build the server
   *
   * @param host the host
   * @param port the port
   * @param poller the poller
   * @return std::expected<impl_server::Server<server_type, R>, std::error_condition>
   */
  std::expected<impl_server::Server<server_type, R>, std::error_condition> build(
      std::string_view host, std::string_view port,
      const std::shared_ptr<sync::Poller>& poller) && {
    auto res = server_type::create(host, port, poller);
    if (!res) {
      return std::unexpected{res.error()};
    }
    return impl_server::Server<server_type, R>{std::move(*res), std::move(details)};
  }

private:
  std::size_t tag;
  std::unique_ptr<details_type> details;
};

XSL_HTTP_NE
#endif
