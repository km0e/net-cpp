#include <netdb.h>
#include <tcp_server.h>
#include <unistd.h>
namespace xsl {
  TcpServer::TcpServer(const char *host, int port)
    : server_fd(-1) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd == -1) {
      return;
    }
    int opt = 1;
    if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
      close(server_fd);
      return;
    }
    sockaddr addr;
    sockaddr_in *addr_in = (sockaddr_in *)&addr;
    addr_in->sin_family = AF_INET;
    addr_in->sin_port = htons(port);
    addr_in->sin_addr.s_addr = INADDR_ANY;
    if(bind(server_fd, &addr, sizeof(addr)) == -1) {
      close(server_fd);
      return;
    }
    if(listen(server_fd, MAX_CONNECTIONS) == -1) {
      close(server_fd);
      return;
    }
    this->server_fd = server_fd;
  }
  TcpServer::~TcpServer() {
    if(this->server_fd != -1) {
      close(this->server_fd);
    }
  }
  bool TcpServer::valid() {
    return this->server_fd != -1;
  }
  void TcpServer::set_handler(Handler handler) {
    this->handler = handler;
  }
  void TcpServer::poller_register(wheel::shared_ptr<Poller> poller) {
    poller->register_handler(this->server_fd, IOM_EVENTS::IN, [poller, this](int fd, IOM_EVENTS events) -> bool {
      if(events & IOM_EVENTS::IN) {
        sockaddr addr;
        socklen_t addr_len = sizeof(addr);
        int client_fd = accept(this->server_fd, &addr, &addr_len);
        if(client_fd == -1) {  // todo: handle error
          return true;
        }
        this->handler(poller, client_fd, IOM_EVENTS::IN);
      }
      return true;
    });
  }
}  // namespace xsl