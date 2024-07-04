#include "xsl/logctl.h"
#include "xsl/net/sync.h"
#include "xsl/net/transport/tcp.h"

#include <CLI/CLI.hpp>
#include <pthread.h>
#include <unistd.h>

#include <string>
#include <thread>
using namespace std;
using namespace xsl::net;
#define MAX_ECHO_CYCLES 10
int main(int argc, char **argv) {
  xsl::no_log();

  CLI::App app{"TCP Client"};
  string ip = "127.0.0.1";
  app.add_option("-i,--ip", ip, "Ip to connect to");
  string port = "12345";
  app.add_option("-p,--port", port, "Port to connect to");
  CLI11_PARSE(app, argc, argv);

  DefaultPoller poller;
  if (!poller.valid()) {
    return 1;
  }
  int echo_cycles = 0;
  auto poller_thread = std::thread([&poller, &echo_cycles]() {
    while (true) {
      poller.poll();
      if (echo_cycles >= MAX_ECHO_CYCLES) {
        break;
      }
    }
    poller.shutdown();
  });
  int fd = transport::new_tcp_client(ip.data(), port.data());
  if (fd < 0) {
    return 1;
  }
  poller.add(fd, IOM_EVENTS::IN, [&echo_cycles](int fd, IOM_EVENTS events) -> PollHandleHint {
    if ((events & IOM_EVENTS::IN) == IOM_EVENTS::IN) {
      char buf[1024];
      int res = read(fd, buf, sizeof(buf));
      if (res < 0) {
        return {PollHandleHintTag::DELETE};
      }
      buf[res] = 0;
      string expected = "Cycle " + to_string(echo_cycles) + ": Hello, world!";
      if (string(buf) != expected) {
        return PollHandleHintTag::DELETE;
      }
      echo_cycles++;
      string msg = "Cycle " + to_string(echo_cycles) + ": Hello, world!";
      res = write(fd, msg.c_str(), msg.size());
      if (res < 0) {
        return PollHandleHintTag::DELETE;
      }
    }
    return PollHandleHintTag::NONE;
  });
  string msg = "Cycle " + to_string(echo_cycles) + ": Hello, world!";
  int res = write(fd, msg.c_str(), msg.size());
  if (res < 0) {
    close(fd);
    return 1;
  }
  poller_thread.join();
  close(fd);
  return 0;
}
