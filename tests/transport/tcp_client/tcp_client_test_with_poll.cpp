#include <spdlog/spdlog.h>
#include <poller.h>
#include <pthread.h>
#include <tcp_client.h>
#include <unistd.h>
#include <wheel.h>
#ifndef TEST_HOST
#define TEST_HOST "localhost"
#endif
#ifndef TEST_PORT
#define TEST_PORT "8080"
#endif
#define MAX_ECHO_CYCLES 10
int main() {
  xsl::Poller poller;
  if(!poller.valid()) {
    return 1;
  }
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
    &poller);
  xsl::TcpClient client;
  int fd = client.connect(TEST_HOST, TEST_PORT);
  if(fd < 0) {
    return 1;
  }
  int echo_cycles = 0;
  poller.register_handler(fd, IOM_EVENTS::IN, [&poller, &echo_cycles](int fd, IOM_EVENTS events) -> bool {
    if(events & IOM_EVENTS::IN) {
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