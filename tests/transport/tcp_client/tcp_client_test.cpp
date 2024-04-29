#include <spdlog/spdlog.h>
#include <tcp_client.h>
#include <unistd.h>
#include <wheel.h>

#ifndef TEST_HOST
#define TEST_HOST "localhost"
#endif
#ifndef TEST_PORT
#define TEST_PORT "8080"
#endif
int main() {
  xsl::TcpClient client;
  int fd = client.connect(TEST_HOST, TEST_PORT);
  if(fd < 0) {
    return 1;
  }
  wheel::string msg = "Hello, world!";
  int res = write(fd, msg.c_str(), msg.size());
  if(res < 0) {
    close(fd);
    spdlog::error("Failed to write to {}:{}", TEST_HOST, TEST_PORT);
    return 1;
  }
  char buf[1024];
  res = read(fd, buf, sizeof(buf));
  if(res < 0) {
    close(fd);
    spdlog::error("Failed to read from {}:{}", TEST_HOST, TEST_PORT);
    return 1;
  }
  buf[res] = 0;
  wheel::string expected = "Hello, world!";
  if(wheel::string(buf) != expected) {
    close(fd);
    spdlog::error("Expected {} but got {}", expected, buf);
    return 1;
  }
  close(fd);
  return 0;
}