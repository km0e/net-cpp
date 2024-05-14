#include "xsl/sync/sync.h"
#include "xsl/transport/tcp/tcp.h"
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
using namespace xsl::transport::tcp;
using namespace xsl::wheel;
using namespace xsl::sync;
class Handler {
public:
  TcpHandleConfig init() {
    TcpHandleConfig config{};
    SPDLOG_DEBUG("[T][Handler::init] Pushing recv task");
    config.recv_tasks.push_front(TcpRecvString::create(this->data));
    return config;
  }
  TcpHandleState recv([[maybe_unused]] TcpRecvTasks &tasks) {
    SPDLOG_INFO("[T][Handler::recv] Received data: {}", this->data);
    return TcpHandleState(IOM_EVENTS::OUT, TcpHandleHint::WRITE);
  }
  TcpHandleState send(TcpSendTasks &tasks) {
    tasks.emplace_front(make_unique<TcpSendString>(xsl::wheel::move(this->data)));
    return TcpHandleState(IOM_EVENTS::NONE, TcpHandleHint::NONE);
  }
  string data;
};
class HandlerGenerator {
public:
  HandlerGenerator() {}
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
  auto poller = make_shared<DefaultPoller>();
  if (!poller->valid()) {
    SPDLOG_ERROR("Failed to create poller");
    return 1;
  }
  auto handler_generator = make_shared<HandlerGenerator>();
  TcpServer<Handler, HandlerGenerator> server{};
  server.set_poller(poller);
  server.set_handler_generator(handler_generator);
  if (!server.serve(ip.c_str(), port)) {
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
