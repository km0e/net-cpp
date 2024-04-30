#include <xsl/sync/poller.h>
#include <xsl/transport/tcp_client.h>
#include <xsl/utils/wheel/wheel.h>

#include <pthread.h>
#include <spdlog/spdlog.h>
#include <unistd.h>
#include <CLI/CLI.hpp>

#define MAX_ECHO_CYCLES 10
int main(int argc, char **argv) {
  CLI::App app{"TCP Client"};
  wheel::string ip = "localhost";
  app.add_option("-i,--ip", ip, "Ip to connect to")->required();
  wheel::string port = "8080";
  app.add_option("-p,--port", port, "Port to connect to")->required();
  CLI11_PARSE(app, argc, argv);

  xsl::sync::Poller poller;
  if(!poller.valid()) {
    return 1;
  }
  pthread_t poller_thread;
  pthread_create(
    &poller_thread, nullptr,
    [](void *arg) -> void * {
      xsl::sync::Poller *poller = (xsl::sync::Poller *)arg;
      while(true) {
        poller->poll();
      }
      return nullptr;
    },
    &poller);
  xsl::transport::TcpClient client;
  int fd = client.connect(ip.c_str(), port.c_str());
  if(fd < 0) {
    return 1;
  }
  int echo_cycles = 0;
  poller.register_handler(fd, xsl::sync::IOM_EVENTS::IN, [&poller, &echo_cycles](int fd, xsl::sync::IOM_EVENTS events) -> bool {
    if(events & xsl::sync::IOM_EVENTS::IN) {
      char buf[1024];
      int res = read(fd, buf, sizeof(buf));
      if(res < 0) {
        return false;
      }
      buf[res] = 0;
      wheel::string expected = "Cycle " + wheel::to_string(echo_cycles) + ": Hello, world!";
      if(wheel::string(buf) != expected) {
        return false;
      }
      echo_cycles++;
      wheel::string msg = "Cycle " + wheel::to_string(echo_cycles) + ": Hello, world!";
      res = write(fd, msg.c_str(), msg.size());
      if(res < 0) {
        return false;
      }
    }
    return true;
  });
  wheel::string msg = "Cycle " + wheel::to_string(echo_cycles) + ": Hello, world!";
  int res = write(fd, msg.c_str(), msg.size());
  if(res < 0) {
    close(fd);
    return 1;
  }
  close(fd);
  return 0;
}