#include <pthread.h>
#include <spdlog/spdlog.h>
#include <sys/signal.h>
#include <unistd.h>
#include <xsl/sync/poller.h>
#include <xsl/transport/tcp/server.h>
#include <xsl/utils/wheel/wheel.h>

#include <CLI/CLI.hpp>

#include "xsl/transport/tcp/helper.h"
#include "xsl/transport/tcp/tcp.h"

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
using TcpHandleState = xsl::transport::tcp::HandleState;
using TcpHandleHint = xsl::transport::tcp::HandleHint;
using xsl::transport::tcp::HandleConfig;
using xsl::transport::tcp::TcpServer;
using TcpTasks = xsl::transport::tcp::Tasks;
class Handler {
public:
  HandleConfig init() {
    HandleConfig config{};
    config.recv_tasks.push_front(wheel::move(wheel::make_unique<xsl::transport::tcp::RecvString>(this->data)));
    return wheel::move(config);
  }
  TcpHandleState recv(TcpTasks &_) {
    spdlog::info("Received data: {}", data);
    return TcpHandleState(xsl::sync::IOM_EVENTS::OUT, TcpHandleHint::WRITE);
  }
  TcpHandleState send(TcpTasks &tasks) {
    tasks.emplace_front(xsl::transport::tcp::SendString::create(wheel::move(this->data)));
    return TcpHandleState(xsl::sync::IOM_EVENTS::NONE, TcpHandleHint::NONE);
  }
  wheel::string data;
};
class HandlerGenerator {
public:
  HandlerGenerator() {}
  ~HandlerGenerator() {}
  Handler operator()() { return {}; }

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
  TcpServer<Handler, HandlerGenerator> server{};
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
