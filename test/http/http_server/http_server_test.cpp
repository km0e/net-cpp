#include <pthread.h>
#include <spdlog/spdlog.h>
#include <sys/signal.h>
#include <unistd.h>
#include <xsl/http/http_msg.h>
#include <xsl/http/http_server.h>
#include <xsl/utils/wheel/wheel.h>

#include <CLI/CLI.hpp>

#include "xsl/http/http_router.h"
#include "xsl/sync/poller.h"
#include "xsl/transport/tcp_server.h"

#ifndef TEST_HOST
#  define TEST_HOST "127.0.0.1"
#endif
#ifndef TEST_PORT
#  define TEST_PORT 8080
#endif
bool recv_handler(int fd, xsl::sync::IOM_EVENTS events);

void sigterm_init() {
  struct sigaction act;
  act.sa_handler = [](int sig) -> void {
    spdlog::info("Received signal {}", sig);
    exit(0);
  };
  sigaction(SIGTERM, &act, nullptr);
  sigaction(SIGINT, &act, nullptr);
}
int main(int argc, char **argv) {
  CLI::App app{"TCP Client"};
  wheel::string ip = "localhost";
  app.add_option("-i,--ip", ip, "Ip to connect to")->required();
  int port = 8080;
  app.add_option("-p,--port", port, "Port to connect to")->required();
  CLI11_PARSE(app, argc, argv);
  spdlog::set_level(spdlog::level::trace);
  sigterm_init();
  auto router = wheel::make_shared<xsl::http::HttpRouter>();
  router->add_route("/", [](xsl::http::HttpRequest req) -> xsl::http::HttpResponse {
    (void)req;
    auto res = xsl::http::HttpResponse{};
    res.version = "HTTP/1.1";
    res.status_code = 200;
    res.status_message = "OK";
    res.body = "Hello, World!";
    return res;
  });
  auto handler_generator = wheel::make_shared<xsl::http::DefaultHandlerGenerator>(router);
  wheel::shared_ptr<xsl::sync::Poller> poller = wheel::make_shared<xsl::sync::EPoller>();
  xsl::transport::TcpServer tcp_server{};
  tcp_server.set_poller(poller);
  tcp_server.set_handler_generator(handler_generator);
  if (!tcp_server.serve(TEST_HOST, TEST_PORT)) {
    spdlog::error("Failed to serve");
    return 1;
  }
  pthread_t poller_thread;
  pthread_create(
      &poller_thread, nullptr,
      [](void *arg) -> void * {
        xsl::sync::EPoller *poller = (xsl::sync::EPoller *)arg;
        while (true) {
          poller->poll();
        }
        return nullptr;
      },
      (void *)poller.get());
  while (true) {
    sleep(1);
  }
  return 0;
}
