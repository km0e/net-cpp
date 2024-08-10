#pragma once

#include <expected>
#include <utility>
#ifndef XSL_NET_HTTP_SERVER
#  define XSL_NET_HTTP_SERVER
#  include "xsl/coro/await.h"
#  include "xsl/coro/task.h"
#  include "xsl/feature.h"
#  include "xsl/logctl.h"
#  include "xsl/net/http/component.h"
#  include "xsl/net/http/def.h"
#  include "xsl/net/http/msg.h"
#  include "xsl/net/http/parse.h"
#  include "xsl/net/http/proto.h"
#  include "xsl/net/http/router.h"
#  include "xsl/net/tcp.h"
#  include "xsl/sync.h"

#  include <memory>
#  include <string_view>
#  include <unordered_map>

HTTP_NB

template <class LowerLayer, RouterLike<std::size_t> R = Router>
class Server {
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

  static std::expected<Server, std::error_condition> create(
      const char* host, const char* port, const std::shared_ptr<sync::Poller>& poller) {
    auto res = _CreateFeature<LowerLayer>::type::create(host, port, poller);
    if (!res) {
      return std::unexpected{res.error()};
    }
    return std::move(*res);
  }

  using server_type = _CreateFeature<LowerLayer>::type;
  using io_dev_type = typename server_type::io_dev_type;
  using in_dev_type = io_dev_type::template rebind_type<feature::In>;
  using out_dev_type = io_dev_type::template rebind_type<feature::Out>;
  using context_type = HandleContext<in_dev_type, out_dev_type>;
  using handler_type = RouteHandler<in_dev_type, out_dev_type>;
  using router_type = R;

  Server(server_type&& server) : Server(std::move(server), std::make_shared<router_type>()) {}
  Server(server_type&& server, std::shared_ptr<router_type> router)
      : server(std::move(server)),
        router(std::move(router)),
        tag(0),
        handlers(std::make_shared<ShardRes<std::unordered_map<std::size_t, handler_type>>>()) {}
  Server(Server&&) = default;
  Server& operator=(Server&&) = default;
  ~Server() {}
  void add_static(std::string_view path) {
    this->add_route(Method::GET, path, create_static_handler<in_dev_type, out_dev_type>(path));
  }

  void add_route(Method method, std::string_view path, handler_type&& handler) {
    auto tag = this->tag.fetch_add(1);
    this->handlers->lock()->try_emplace(tag, std::move(handler));
    this->router->add_route(method, path, std::move(tag));
  }

  void set_error_handler(RouteError kind, handler_type&& handler) {
    this->error_handlers[static_cast<uint8_t>(kind)] = std::move(handler);
  }

  template <class Executor = coro::ExecutorBase>
  coro::Task<std::expected<void, std::errc>, Executor> run() {
    LOG4("HttpServer start");
    while (true) {
      auto res = co_await this->server.template accept<Executor>(nullptr);
      if (!res) {
        LOG2("accept error: {}", std::make_error_code(res.error()).message());
        continue;
      }
      http_connection<Executor>(std::move(*res), this->router, this->handlers)
          .detach(co_await coro::GetExecutor<Executor>());
    }
  }

private:
  server_type server;
  std::shared_ptr<router_type> router;
  std::atomic<std::size_t> tag;
  std::shared_ptr<ShardRes<std::unordered_map<std::size_t, handler_type>>> handlers;
  std::array<handler_type, ROUTE_ERROR_COUNT> error_handlers
      = {UNKNOWN_HANDLER<in_dev_type, out_dev_type>, NOT_FOUND_HANDLER<in_dev_type, out_dev_type>,
         NOT_IMPLEMENTED_HANDLER<in_dev_type, out_dev_type>};

  template <class Executor = coro::ExecutorBase>
  coro::Task<void, Executor> http_connection(
      io_dev_type dev, std::shared_ptr<router_type> router,
      std::shared_ptr<ShardRes<std::unordered_map<std::size_t, handler_type>>> handlers) {
    auto [ard, awd] = std::move(dev).split();
    auto parser = Parser<HttpParseTrait>{};
    ParseData parse_data{};
    while (true) {
      {
        auto res = co_await parser.template read<Executor>(ard, parse_data);
        if (!res) {
          LOG3("recv error: {}", std::make_error_code(res.error()).message());
          continue;
        }
      }

      bool keep_alive = true;
      if (auto iter = parse_data.request.headers.find("Connection");
          iter == parse_data.request.headers.end() || iter->second != "keep-alive") {
        keep_alive = false;
      }

      Request<in_dev_type> request{std::move(parse_data.buffer), std::move(parse_data.request),
                                   parse_data.content_part, ard};

      auto route_ctx = RouteContext{request.method, request.view.path};

      auto route_res = router->route(route_ctx);
      auto ctx = context_type{route_ctx.current_path, std::move(request)};
      handler_type* handler = nullptr;
      if (!route_res) {
        handler = &error_handlers[static_cast<uint8_t>(route_res.error())];
      } else {
        handler = &handlers->lock_shared()->at(**route_res);
      }
      co_await (*handler)(ctx);
      if (!ctx._response) {
        LOG3("response is null");
        continue;
      }
      auto [sz, err] = co_await ctx._response->sendto(awd);
      if (err) {
        LOG3("send error: {}", std::make_error_code(*err).message());
      }
      if (!keep_alive) {
        break;
      }
    }
  }
};
HTTP_NE
#endif
