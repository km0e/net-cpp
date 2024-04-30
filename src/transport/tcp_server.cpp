#include <arpa/inet.h>
#include <netdb.h>
#include <spdlog/spdlog.h>
#include <unistd.h>
#include <xsl/config.h>
#include <xsl/transport/tcp_server.h>
#include <xsl/transport/transport.h>
XSL_NAMESPACE_BEGIN
TRANSPORT_NAMESPACE_BEGIN
TcpServer::TcpServer()
  : server_fd(-1) {
}
bool TcpServer::serve(const char *host, int port) {
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if(server_fd == -1) {
    spdlog::error("Failed to create socket");
    return false;
  }
  int opt = 1;
  if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
    close(server_fd);
    spdlog::error("Failed to set socket options");
    return false;
  }
  sockaddr addr;
  sockaddr_in *addr_in = (sockaddr_in *)&addr;
  addr_in->sin_family = AF_INET;
  addr_in->sin_port = htons(port);
  addr_in->sin_addr.s_addr = inet_addr(host);
  if(bind(server_fd, &addr, sizeof(addr)) == -1) {
    close(server_fd);
    spdlog::error("Failed to bind to {}:{}", host, port);
    return false;
  }
  if(listen(server_fd, MAX_CONNECTIONS) == -1) {
    close(server_fd);
    spdlog::error("Failed to listen on {}:{}", host, port);
    return false;
  }
  this->server_fd = server_fd;
  return true;
}
TcpServer::~TcpServer() {
  if(this->server_fd != -1) {
    close(this->server_fd);
  }
}
bool TcpServer::valid() {
  return this->server_fd != -1;
}
void TcpServer::set_handler(sync::Handler handler) {
  this->handler = handler;
}
void TcpServer::poller_register(wheel::shared_ptr<sync::Poller> poller) {
  poller->register_handler(this->server_fd, sync::IOM_EVENTS::IN, [poller, this](int fd, sync::IOM_EVENTS events) -> bool {
    if(events & sync::IOM_EVENTS::IN) {
      spdlog::info("Accepting connection");
      sockaddr addr;
      socklen_t addr_len = sizeof(addr);
      int client_fd = accept(this->server_fd, &addr, &addr_len);
      if(client_fd == -1) {  // todo: handle error
        return true;
      }
      this->handler(poller, client_fd, sync::IOM_EVENTS::IN);
    }
    return true;
  });
}
TRANSPORT_NAMESPACE_END
XSL_NAMESPACE_END