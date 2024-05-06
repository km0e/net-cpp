#include <spdlog/spdlog.h>
#include <unistd.h>
#include <xsl/utils/wheel/wheel.h>

#include <CLI/CLI.hpp>

#include "xsl/transport/utils.h"

int main(int argc, char **argv) {
  CLI::App app{"TCP Client"};
  wheel::string ip = "127.0.0.1";
  app.add_option("-i,--ip", ip, "Ip to connect to")->required();
  wheel::string port = "8080";
  app.add_option("-p,--port", port, "Port to connect to")->required();
  CLI11_PARSE(app, argc, argv);

  int fd = xsl::transport::create_tcp_client(ip.c_str(), port.c_str());
  if (fd < 0) {
    return 1;
  }
  wheel::string msg = "Hello, world!";
  int res = write(fd, msg.c_str(), msg.size());
  if (res < 0) {
    close(fd);
    spdlog::error("Failed to write to {}:{}", ip, port);
    return 1;
  }
  char buf[1024];
  res = read(fd, buf, sizeof(buf));
  if (res < 0) {
    close(fd);
    spdlog::error("Failed to read from {}:{}", ip, port);
    return 1;
  }
  buf[res] = 0;
  wheel::string expected = "Hello, world!";
  if (wheel::string(buf) != expected) {
    close(fd);
    spdlog::error("Expected {} but got {}", expected, buf);
    return 1;
  }
  close(fd);
  return 0;
}
