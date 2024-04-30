#include <sys/signal.h>
#include <pthread.h>
#include <spdlog/spdlog.h>
#include <xsl/transport/tcp_server.h>
#include <xsl/sync/poller.h>
#include <xsl/utils/wheel/wheel.h>
#include <unistd.h>

#include <CLI/CLI.hpp>

#ifndef TEST_HOST
#define TEST_HOST "localhost"
#endif
#ifndef TEST_PORT
#define TEST_PORT 8080
#endif
bool recv_handler(int fd, xsl::IOM_EVENTS events);
void sigterm_init() {
  struct sigaction act;
  act.sa_handler = [](int sig) -> void {
    spdlog::info("Received signal {}", sig);
    exit(0);
  };
  sigaction(SIGTERM, &act, nullptr);
  sigaction(SIGINT, &act, nullptr);
}
int main(int argc, char **argv) {
  CLI::App app{"TCP Client"};
  wheel::string ip = "localhost";
  app.add_option("-i,--ip", ip, "Ip to connect to")->required();
  int port = 8080;
  app.add_option("-p,--port", port, "Port to connect to")->required();
  CLI11_PARSE(app, argc, argv);

  sigterm_init();

  xsl::TcpServer server(ip.c_str(), port);
  if(!server.valid()) {
    spdlog::error("Failed to create server on {}:{}", ip, port);
    return 1;
  }
  server.set_handler([&server](wheel::shared_ptr<xsl::Poller> poller, int fd, xsl::IOM_EVENTS events) -> bool {
    spdlog::info("New connection");
    if(events & xsl::IOM_EVENTS::IN) {
      poller->register_handler(fd, xsl::IOM_EVENTS::IN, recv_handler);
    }
    return true;
  });
  auto poller = wheel::make_shared<xsl::Poller>();
  if(!poller->valid()) {
    spdlog::error("Failed to create poller");
    return 1;
  }
  server.poller_register(poller);
  pthread_t poller_thread;
  pthread_create(
    &poller_thread, nullptr,
    [](void *arg) -> void * {
      xsl::Poller *poller = (xsl::Poller *)arg;
      while(true) {
        poller->poll();
      }
      return nullptr;
    },
    poller.get());
  while(true) {
    sleep(1);
  }
  return 0;
}

bool recv_handler(int fd, xsl::IOM_EVENTS events) {
  if(events & xsl::IOM_EVENTS::IN) {
    char buf[1024];
    while(true) {
      int res = read(fd, buf, sizeof(buf));
      if(res < 0) {
        close(fd);
        return false;
      }
      if(res == 0) {
        break;
      }
      buf[res] = 0;
      spdlog::info("Received: {}", buf);
      while(true) {
        int wres = write(fd, buf, res);
        if(wres < 0) {
          close(fd);
          return false;
        }
        spdlog::info("Sent: {}", buf);
        if(wres == res) {
          break;
        }
        res -= wres;
      }
    }
  }
  return true;
}
