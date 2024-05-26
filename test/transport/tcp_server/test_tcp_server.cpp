#include "xsl/net.h"
#include "xsl/wheel.h"

#include <CLI/CLI.hpp>
#include <pthread.h>
#include <spdlog/spdlog.h>
#include <sys/signal.h>
#include <unistd.h>

#ifndef TEST_HOST
#  define TEST_HOST "127.0.0.1"
#endif
#ifndef TEST_PORT
#  define TEST_PORT 8080
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
using namespace xsl;
class Handler {
public:
  TcpHandleConfig init([[maybe_unused]]int fd) {
    TcpHandleConfig config{};
    SPDLOG_DEBUG("[T][Handler::init] Pushing recv task");
    config.recv_tasks.push_front(make_unique<TcpRecvString>(this->data));
    return config;
  }
  TcpHandleState recv([[maybe_unused]] TcpRecvTasks &tasks) {
    SPDLOG_INFO("[T][Handler::recv] Received data: {}", this->data);
    return TcpHandleState(IOM_EVENTS::OUT, TcpHandleHint::WRITE);
  }
  TcpHandleState send(TcpSendTasks &tasks) {
    tasks.emplace_front(make_unique<TcpSendString>(xsl::move(this->data)));
    return TcpHandleState(IOM_EVENTS::NONE, TcpHandleHint::NONE);
  }
  string data;
};
class HandlerGenerator {
public:
  HandlerGenerator() : data("Hello, world!") {}
  ~HandlerGenerator() {}
  Handler operator()() { return {}; }

private:
  string data;
};

int main(int argc, char **argv) {
  CLI::App app{"TCP Client"};
  string ip = "localhost";
  app.add_option("-i,--ip", ip, "Ip to connect to")->required();
  int port = 8080;
  app.add_option("-p,--port", port, "Port to connect to")->required();
  CLI11_PARSE(app, argc, argv);

  sigterm_init();

  spdlog::set_level(spdlog::level::trace);
  TcpServerConfig<Handler, HandlerGenerator> config{};
  config.max_connections = 10;
  config.host = ip;
  config.port = port;
  auto poller = make_shared<DefaultPoller>();
  if (!poller->valid()) {
    SPDLOG_ERROR("Failed to create poller");
    return 1;
  }
  config.poller = poller;
  auto handler_generator = make_shared<HandlerGenerator>();
  config.handler_generator = handler_generator;
  auto server = TcpServer<Handler, HandlerGenerator>::serve(config);
  if (!server) {
    SPDLOG_ERROR("Failed to create server on {}:{}", ip, port);
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
      poller.get());
  while (true) {
    sleep(1);
  }
  return 0;
}
