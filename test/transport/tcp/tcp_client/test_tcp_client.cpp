#include "xsl/logctl.h"
#include "xsl/net/transport/tcp.h"

#include <CLI/CLI.hpp>
#include <unistd.h>

#include <string>

using namespace std;
using namespace xsl::net;
int main(int argc, char **argv) {
  xsl::no_log();
  CLI::App app{"TCP Client"};
  string ip = "127.0.0.1";
  app.add_option("-i,--ip", ip, "Ip to connect to");
  string port = "12346";
  app.add_option("-p,--port", port, "Port to connect to");
  CLI11_PARSE(app, argc, argv);

  int fd = transport::new_tcp_client(ip.data(), port.data());
  if (fd < 0) {
    return 1;
  }
  string msg = "Hello, world!";
  int res = write(fd, msg.c_str(), msg.size());
  if (res < 0) {
    close(fd);
    ERROR("Failed to write to {}:{}", ip, port);
    return 1;
  }
  char buf[1024];
  res = read(fd, buf, sizeof(buf));
  if (res < 0) {
    close(fd);
    ERROR("Failed to read from {}:{}", ip, port);
    return 1;
  }
  buf[res] = 0;
  string expected = "Hello, world!";
  if (string(buf) != expected) {
    close(fd);
    ERROR("Expected {} but got {}", expected, buf);
    return 1;
  }
  close(fd);
  return 0;
}
