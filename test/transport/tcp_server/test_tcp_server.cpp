#include <pthread.h>
#include <spdlog/spdlog.h>
#include <sys/signal.h>
#include <unistd.h>
#include <xsl/sync/poller.h>
#include <xsl/transport/server.h>
#include <xsl/utils/wheel/wheel.h>

#include <CLI/CLI.hpp>

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
class Handler {
public:
  xsl::transport::HandleState handle(int r_w, wheel::string &data) {
    (void)data;
    if (r_w == 0) {
      return xsl::transport::HandleState(xsl::sync::IOM_EVENTS::OUT,
                                         xsl::transport::HandleHint::WRITE);
    } else {
      return xsl::transport::HandleState(xsl::sync::IOM_EVENTS::NONE,
                                         xsl::transport::HandleHint::NONE);
    }
  }
};
class HandlerGenerator {
public:
  HandlerGenerator() {}
  ~HandlerGenerator() {}
  Handler generate() { return {}; }

private:
  wheel::string data;
};

int main(int argc, char **argv) {
  CLI::App app{"TCP Client"};
  wheel::string ip = "localhost";
  app.add_option("-i,--ip", ip, "Ip to connect to")->required();
  int port = 8080;
  app.add_option("-p,--port", port, "Port to connect to")->required();
  CLI11_PARSE(app, argc, argv);

  sigterm_init();

  spdlog::set_level(spdlog::level::trace);
  auto poller = wheel::make_shared<xsl::sync::EPoller>();
  if (!poller->valid()) {
    spdlog::error("Failed to create poller");
    return 1;
  }
  auto handler_generator = wheel::make_shared<HandlerGenerator>();
  xsl::transport::TcpServer<Handler, HandlerGenerator> server{};
  server.set_poller(poller);
  server.set_handler_generator(handler_generator);
  if (!server.serve(ip.c_str(), port)) {
    spdlog::error("Failed to create server on {}:{}", ip, port);
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
      poller.get());
  while (true) {
    sleep(1);
  }
  return 0;
}
