#include <spdlog/spdlog.h>
#include <tcp_server.h>
#include <unistd.h>
#include <wheel.h>

#include "poller.h"

#ifndef TEST_HOST
#define TEST_HOST "localhost"
#endif
#ifndef TEST_PORT
#define TEST_PORT 8080
#endif
bool recv_handler(int fd, IOM_EVENTS events) {
  if(events & IOM_EVENTS::IN) {
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
      while(true) {
        int wres = write(fd, buf, res);
        if(wres < 0) {
          close(fd);
          return false;
        }
        if(wres == res) {
          break;
        }
        res -= wres;
      }
    }
  }
  return true;
}

int main() {
  xsl::TcpServer server(TEST_HOST, TEST_PORT);
  if(!server.valid()) {
    spdlog::error("Failed to create server on {}:{}", TEST_HOST, TEST_PORT);
    return 1;
  }
  server.set_handler([&server](wheel::shared_ptr<xsl::Poller> poller, int fd, IOM_EVENTS events) -> bool {
    if(events & IOM_EVENTS::IN) {
      poller->register_handler(fd, IOM_EVENTS::IN, recv_handler);
    }
    return true;
  });
  auto poller = wheel::make_shared<xsl::Poller>();
  if(!poller->valid()) {
    spdlog::error("Failed to create poller");
    return 1;
  }
  server.poller_register(poller);
  while(true) {
    poller->poll();
  }
  return 0;
}