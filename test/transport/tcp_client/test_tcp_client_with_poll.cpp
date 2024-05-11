#include <pthread.h>
#include <spdlog/spdlog.h>
#include <unistd.h>

#include <CLI/CLI.hpp>

#include "xsl/sync/poller.h"
#include "xsl/transport/utils.h"
#include "xsl/utils/wheel/wheel.h"

#define MAX_ECHO_CYCLES 10
int main(int argc, char **argv) {
  CLI::App app{"TCP Client"};
  wheel::string ip = "127.0.0.1";
  app.add_option("-i,--ip", ip, "Ip to connect to")->required();
  wheel::string port = "8080";
  app.add_option("-p,--port", port, "Port to connect to")->required();
  CLI11_PARSE(app, argc, argv);

  xsl::sync::EPoller poller;
  if (!poller.valid()) {
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
      &poller);
  int fd = xsl::transport::create_tcp_client(ip.c_str(), port.c_str());
  if (fd < 0) {
    return 1;
  }
  int echo_cycles = 0;
  poller.subscribe(
      fd, xsl::sync::IOM_EVENTS::IN,
      [&echo_cycles](int fd, xsl::sync::IOM_EVENTS events) -> xsl::sync::IOM_EVENTS {
        if ((events & xsl::sync::IOM_EVENTS::IN) == xsl::sync::IOM_EVENTS::IN) {
          char buf[1024];
          int res = read(fd, buf, sizeof(buf));
          if (res < 0) {
            return xsl::sync::IOM_EVENTS::NONE;
          }
          buf[res] = 0;
          wheel::string expected = "Cycle " + wheel::to_string(echo_cycles) + ": Hello, world!";
          if (wheel::string(buf) != expected) {
            return xsl::sync::IOM_EVENTS::NONE;
          }
          echo_cycles++;
          wheel::string msg = "Cycle " + wheel::to_string(echo_cycles) + ": Hello, world!";
          res = write(fd, msg.c_str(), msg.size());
          if (res < 0) {
            return xsl::sync::IOM_EVENTS::NONE;
          }
        }
        return xsl::sync::IOM_EVENTS::IN;
      });
  wheel::string msg = "Cycle " + wheel::to_string(echo_cycles) + ": Hello, world!";
  int res = write(fd, msg.c_str(), msg.size());
  if (res < 0) {
    close(fd);
    return 1;
  }
  close(fd);
  return 0;
}
