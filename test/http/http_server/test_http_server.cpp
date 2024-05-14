#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE

#include "xsl/net/http/http.h"
#include "xsl/net/sync/sync.h"
#include "xsl/wheel/wheel.h"

#include <CLI/CLI.hpp>
#include <pthread.h>
#include <spdlog/spdlog.h>
#include <sys/signal.h>
#include <unistd.h>

#ifndef TEST_HOST
#  define TEST_HOST "127.0.0.1"
#endif
#ifndef TEST_PORT
#  define TEST_PORT 12345
#endif
void sigterm_init() {
  struct sigaction act;
  act.sa_handler = [](int sig) -> void {
    SPDLOG_INFO("Received signal {}", sig);
    exit(0);
  };
  sigaction(SIGTERM, &act, nullptr);
  sigaction(SIGINT, &act, nullptr);
}
using namespace xsl::net::http;
using namespace xsl::wheel;
using namespace xsl::net::sync;
int main(int argc, char **argv) {
  CLI::App app{"TCP Client"};
  string ip = TEST_HOST;
  app.add_option("-i,--ip", ip, "Ip to connect to");
  int port = TEST_PORT;
  app.add_option("-p,--port", port, "Port to connect to");
  CLI11_PARSE(app, argc, argv);
  spdlog::set_level(spdlog::level::trace);
  sigterm_init();

  DefaultServerConfig config{};
  config.max_connections = 10;
  config.host = ip;
  config.port = port;
  auto router = make_shared<DefaultRouter>();
  router->add_route(HttpMethod::GET, "/",
                    create_static_handler("test/http/http_server/web/static/").unwrap());
  router->error_handler(
      RouteErrorKind::NotFound,
      create_static_handler("test/http/http_server/web/static/404.html").unwrap());
  router->error_handler(
      RouteErrorKind::Unimplemented,
      create_static_handler("test/http/http_server/web/static/501.html").unwrap());
  auto handler_generator = make_shared<DefaultHandlerGenerator>(router);
  config.handler_generator = handler_generator;
  xsl::wheel::shared_ptr<DefaultPoller> poller = make_shared<DefaultPoller>();
  if (!poller->valid()) {
    SPDLOG_ERROR("Failed to create poller");
    return 1;
  }
  config.poller = poller;
  auto server = DefaultServer::serve(config);
  if (!server) {
    SPDLOG_ERROR("Failed to serve");
    return 1;
  }
  pthread_t poller_thread;
  pthread_create(
      &poller_thread, nullptr,
      [](void *arg) -> void * {
        DefaultPoller *poller = (DefaultPoller *)arg;
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
