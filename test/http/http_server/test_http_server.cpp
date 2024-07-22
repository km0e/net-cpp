#include "xsl/logctl.h"
#include "xsl/net.h"

#include <CLI/CLI.hpp>
#include <pthread.h>
#include <sys/signal.h>
#include <unistd.h>

#ifndef TEST_HOST
#  define TEST_HOST "127.0.0.1"
#endif
#ifndef TEST_PORT
#  define TEST_PORT "12345"
#endif
void sigterm_init() {
  struct sigaction act;
  act.sa_handler = [](int sig) -> void {
    INFO("Received signal {}", sig);
    exit(0);
  };
  sigaction(SIGTERM, &act, nullptr);
  sigaction(SIGINT, &act, nullptr);
}
using namespace xsl;
int main(int argc, char **argv) {
  CLI::App app{"TCP Client"};
  std::string ip = TEST_HOST;
  app.add_option("-i,--ip", ip, "Ip to connect to");
  std::string port = TEST_PORT;
  app.add_option("-p,--port", port, "Port to connect to");
  CLI11_PARSE(app, argc, argv);
  sigterm_init();

  auto router = std::make_shared<HttpRouter>();
  router->add_route(HttpMethod::GET, "/",
                    create_static_handler("test/http/http_server/web/static/").unwrap());
  router->error_handler(
      RouteErrorKind::NotFound,
      create_static_handler("test/http/http_server/web/static/404.html").unwrap());
  router->error_handler(
      RouteErrorKind::Unimplemented,
      create_static_handler("test/http/http_server/web/static/501.html").unwrap());
  SockAddrV4 ip_port{ip, port};
  auto handler_generator = make_shared<HttpHandlerGenerator>(router);
  auto poller = std::make_shared<DefaultPoller>();
  if (!poller->valid()) {
    LOG2("Failed to create poller");
    return 1;
  }
  TcpConnManagerConfig config{poller};
  auto server = make_unique<HttpServer>(handler_generator, config);
  if (!server) {
    LOG2("Failed to serve");
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
