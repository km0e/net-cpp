#include "xsl/sync/poller.h"
#include "xsl/sync/sync.h"
#include "xsl/transport/utils.h"

#include <CLI/CLI.hpp>
#include <pthread.h>
#include <spdlog/spdlog.h>
#include <unistd.h>
using namespace xsl::wheel;
using namespace xsl::sync;
#define MAX_ECHO_CYCLES 10
int main(int argc, char **argv) {
  CLI::App app{"TCP Client"};
  string ip = "127.0.0.1";
  app.add_option("-i,--ip", ip, "Ip to connect to")->required();
  string port = "8080";
  app.add_option("-p,--port", port, "Port to connect to")->required();
  CLI11_PARSE(app, argc, argv);

  DefaultPoller poller;
  if (!poller.valid()) {
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
      &poller);
  int fd = xsl::transport::create_tcp_client(ip.c_str(), port.c_str());
  if (fd < 0) {
    return 1;
  }
  int echo_cycles = 0;
  poller.subscribe(fd, IOM_EVENTS::IN, [&echo_cycles](int fd, IOM_EVENTS events) -> IOM_EVENTS {
    if ((events & IOM_EVENTS::IN) == IOM_EVENTS::IN) {
      char buf[1024];
      int res = read(fd, buf, sizeof(buf));
      if (res < 0) {
        return IOM_EVENTS::NONE;
      }
      buf[res] = 0;
      string expected = "Cycle " + to_string(echo_cycles) + ": Hello, world!";
      if (string(buf) != expected) {
        return IOM_EVENTS::NONE;
      }
      echo_cycles++;
      string msg = "Cycle " + to_string(echo_cycles) + ": Hello, world!";
      res = write(fd, msg.c_str(), msg.size());
      if (res < 0) {
        return IOM_EVENTS::NONE;
      }
    }
    return IOM_EVENTS::IN;
  });
  string msg = "Cycle " + to_string(echo_cycles) + ": Hello, world!";
  int res = write(fd, msg.c_str(), msg.size());
  if (res < 0) {
    close(fd);
    return 1;
  }
  close(fd);
  return 0;
}
