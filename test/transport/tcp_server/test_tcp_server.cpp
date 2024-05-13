#include "xsl/sync/poller.h"
#include "xsl/transport/tcp/helper.h"
#include "xsl/transport/tcp/server.h"
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
using TcpHandleState = xsl::transport::tcp::HandleState;
using TcpHandleHint = xsl::transport::tcp::HandleHint;
using xsl::transport::tcp::HandleConfig;
using xsl::transport::tcp::TcpServer;
using SendTasks = xsl::transport::tcp::SendTasks;
using RecvTasks = xsl::transport::tcp::RecvTasks;
using xsl::wheel::string;
class Handler {
public:
  HandleConfig init() {
    HandleConfig config{};
    SPDLOG_DEBUG("[T][Handler::init] Pushing recv task");
    config.recv_tasks.push_front(xsl::transport::tcp::RecvString::create(this->data));
    return config;
  }
  TcpHandleState recv([[maybe_unused]] RecvTasks &tasks) {
    SPDLOG_INFO("[T][Handler::recv] Received data: {}", this->data);
    return TcpHandleState(xsl::sync::IOM_EVENTS::OUT, TcpHandleHint::WRITE);
  }
  TcpHandleState send(SendTasks &tasks) {
    tasks.emplace_front(
        xsl::wheel::make_unique<xsl::transport::tcp::SendString>(xsl::wheel::move(this->data)));
    return TcpHandleState(xsl::sync::IOM_EVENTS::NONE, TcpHandleHint::NONE);
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
  auto poller = xsl::wheel::make_shared<xsl::sync::EPoller>();
  if (!poller->valid()) {
    SPDLOG_ERROR("Failed to create poller");
    return 1;
  }
  auto handler_generator = xsl::wheel::make_shared<HandlerGenerator>();
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
