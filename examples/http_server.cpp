
#include <CLI/CLI.hpp>
#include <xsl/coro.h>
#include <xsl/logctl.h>
#include <xsl/net.h>

std::string ip = "127.0.0.1";
std::string port = "8080";

using namespace xsl::net;
using namespace xsl::feature;
using namespace xsl::coro;
using namespace xsl;

template <class Executor = ExecutorBase, bool DynamicService = false,
          bool DynamicConnection = false>
Lazy<void, Executor> run(std::string_view ip, std::string_view port,
                         std::shared_ptr<xsl::Poller> poller) {
  using Server = tcp::Server<Ip<4>>;
  auto server = Server::create(ip, port, poller).value();
  auto service = [&]() {
    if constexpr (DynamicService) {
      return http1::make_service();
    } else {
      return http1::make_service<Server::io_dev_type>();
    }
  }();
  service.redirect(http::Method::GET, "/", "/index.html");
  service.add_static("/", {"./build/html/", {"br"}});
  auto http_service = std::move(service).build_shared();
  while (true) {
    auto skt = co_await [&]() {
      if constexpr (DynamicConnection) {
        return server.accept();
      } else {
        return server.accept<Executor>();
      }
    }();
    if (!skt) {
      LOG3("accept error: {}", std::make_error_code(skt.error()).message());
      break;
    }
    INFO("New connection is accepted");
    auto conn = http1::make_connection(std::move(*skt));
    std::move(conn)
        .template serve_connection<Executor>(http_service)  // same as accept, also can be dynamic
        .detach(co_await coro::GetExecutor<Executor>());
  }
  poller->shutdown();
  co_return;
}

template <class Executor = ExecutorBase, bool DynamicService = false,
          bool DynamicConnection = false>
Lazy<void, Executor> run_step(std::string_view ip, std::string_view port,
                              std::shared_ptr<xsl::Poller> poller) {
  using Server = tcp::Server<Ip<4>>;
  auto server = Server::create(ip, port, poller).value();
  auto http_server = http1::Server{std::move(server)};
  auto service = [&]() {
    if constexpr (DynamicService) {
      return http1::make_service();
    } else {
      return http1::make_service<Server::io_dev_type>();
    }
  }();
  service.redirect(http::Method::GET, "/", "/index.html");
  service.add_static("/", {"./build/html/", {"br"}});
  auto http_service = std::move(service).build();
  co_await [&]() {
    if constexpr (DynamicConnection) {
      return http_server.serve_connection(std::move(http_service));
    } else {
      return http_server.serve_connection<Executor>(std::move(http_service));
    }
  }();
  poller->shutdown();
  co_return;
}

int main(int argc, char* argv[]) {
  CLI::App app{"Echo server"};
  app.add_option("-i,--ip", ip, "IP address");
  app.add_option("-p,--port", port, "Port");
  CLI11_PARSE(app, argc, argv);

  auto poller = std::make_shared<xsl::Poller>();
  auto executor = std::make_shared<NewThreadExecutor>();
  // run(ip, port, poller).detach();
  run<NewThreadExecutor>(ip, port, poller).detach(std::move(executor));
  // run<NewThreadExecutor, false, true>(ip, port, poller).detach(std::move(executor));
  // run<NewThreadExecutor, true, false>(ip, port, poller).detach(std::move(executor));
  // run<NewThreadExecutor, true, true>(ip, port, poller).detach(std::move(executor));
  // run_step<NewThreadExecutor>(ip, port, poller).detach(std::move(executor));
  // run_step<NewThreadExecutor, false, true>(ip, port, poller).detach(std::move(executor));
  // run_step<NewThreadExecutor, true, false>(ip, port, poller).detach(std::move(executor));
  // run_step<NewThreadExecutor, true, true>(ip, port, poller).detach(std::move(executor));
  while (poller->valid()) {
    poller->poll();
  }
  return 0;
}
