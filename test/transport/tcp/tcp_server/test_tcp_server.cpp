#include "xsl/feature.h"
#include "xsl/net/sync.h"
#include "xsl/net/transport/tcp.h"

#include <CLI/CLI.hpp>
#include <pthread.h>
#include <spdlog/spdlog.h>
#include <sys/signal.h>
#include <unistd.h>

#include <memory>

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
using namespace xsl::net;
using namespace xsl::net::transport;
class Handler {
public:
  Handler() : data(), recv_task(), send_tasks() {}
  TcpHandleState recv(int fd) {
    auto res = recv_task.exec(fd);
    if (res.is_err()) {
      SPDLOG_ERROR("recv error: {}", to_string_view(res.unwrap_err()));
      return TcpHandleState::CLOSE;
    }
    SPDLOG_INFO("[T][Handler::recv] Received data: {}", this->recv_task.data_buffer);
    this->send_tasks.tasks.emplace_after(
        this->send_tasks.tasks.before_begin(),
        make_unique<TcpSendString<feature::node>>(std::move(this->recv_task.data_buffer)));
    this->send_tasks.exec(fd);
    return TcpHandleState::NONE;
  }
  TcpHandleState send([[maybe_unused]] int fd) { return TcpHandleState::NONE; }
  void close([[maybe_unused]] int fd){};
  TcpHandleState other(int fd, [[maybe_unused]] IOM_EVENTS events) {
    SPDLOG_INFO("[T][Handler::other]");
    this->send_tasks.exec(fd);
    return TcpHandleState::NONE;
  }
  std::string data;
  TcpRecvString<> recv_task;
  TcpSendTasksProxy send_tasks;
};
class HandlerGenerator {
public:
  HandlerGenerator() : data("Hello, world!") {}
  ~HandlerGenerator() {}
  std::unique_ptr<Handler> operator()([[maybe_unused]] int fd) {
    return std::make_unique<Handler>();
  }

private:
  std::string data;
};

int main(int argc, char **argv) {
  CLI::App app{"TCP Client"};
  std::string ip = "localhost";
  app.add_option("-i,--ip", ip, "Ip to connect to")->required();
  std::string port = "8080";
  app.add_option("-p,--port", port, "Port to connect to")->required();
  CLI11_PARSE(app, argc, argv);

  sigterm_init();

  spdlog::set_level(spdlog::level::trace);
  SockAddrV4 sa4(ip, port);
  int fd = new_tcp_server(sa4);
  auto poller = std::make_shared<DefaultPoller>();
  if (!poller->valid()) {
    SPDLOG_ERROR("Failed to create poller");
    return 1;
  }
  auto handler_generator = std::make_shared<HandlerGenerator>();
  TcpConnManagerConfig config{poller};
  auto server = std::make_unique<TcpServer<Handler, HandlerGenerator>>(handler_generator, config);
  if (!server) {
    SPDLOG_ERROR("Failed to create server on {}:{}", ip, port);
    return 1;
  }
  poller->add(fd, IOM_EVENTS::IN,
              [server = server.get()]([[maybe_unused]] int fd, IOM_EVENTS events) {
                return (*server)(fd, events);
              });

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
