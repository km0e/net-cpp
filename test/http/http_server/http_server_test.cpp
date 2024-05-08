#include <pthread.h>
#include <spdlog/spdlog.h>
#include <sys/signal.h>
#include <unistd.h>
#include <xsl/http/msg.h>
#include <xsl/http/router.h>
#include <xsl/http/server.h>
#include <xsl/sync/poller.h>
#include <xsl/transport/tcp/server.h>
#include <xsl/utils/wheel/wheel.h>

#include <CLI/CLI.hpp>

#include "xsl/http/context.h"

#ifndef TEST_HOST
#  define TEST_HOST "127.0.0.1"
#endif
#ifndef TEST_PORT
#  define TEST_PORT 8080
#endif
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
  auto router = wheel::make_shared<xsl::http::DefaultRouter>();
  router->add_route(xsl::http::HttpMethod::GET, "/",
                    [](xsl::http::Context &ctx) -> xsl::http::Response {
                      (void)ctx;
                      auto res = xsl::http::Response{};
                      res.version = "HTTP/1.1";
                      res.status_code = 200;
                      res.status_message = "OK";
                      res.body = "Hello, World!";
                      return res;
                    });
  auto handler_generator = wheel::make_shared<xsl::http::DefaultHG>(router);
  wheel::shared_ptr<xsl::sync::Poller> poller = wheel::make_shared<xsl::sync::EPoller>();
  xsl::http::DefaultServer tcp_server{};
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
