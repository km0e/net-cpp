#pragma once
#ifndef _XSL_NET_TRANSPORT_UTILS_H_
#  define _XSL_NET_TRANSPORT_UTILS_H_
#  include "xsl/coro/task.h"
#  include "xsl/net/sync.h"
#  include "xsl/net/transport/resolve.h"
#  include "xsl/net/transport/tcp/def.h"
#  include "xsl/wheel.h"

#  include <expected>

TCP_NAMESPACE_BEGIN
class SockAddrV4View {
public:
  SockAddrV4View() = default;
  SockAddrV4View(const char *ip, const char *port);
  // eg: "0.0.0.1:80"
  SockAddrV4View(const char *sa4);
  SockAddrV4View(std::string_view sa4);
  SockAddrV4View(std::string_view ip, std::string_view port);
  bool operator==(const SockAddrV4View &rhs) const;
  const char *ip() const;
  const char *port() const;
  std::string to_string() const;
  std::string_view _ip;
  std::string_view _port;
};

class SockAddrV4 {
public:
  SockAddrV4() = default;
  SockAddrV4(int fd);
  SockAddrV4(const char *ip, const char *port);
  SockAddrV4(std::string_view ip, std::string_view port);
  SockAddrV4(SockAddrV4View sa4);
  SockAddrV4View view() const;
  bool operator==(const SockAddrV4View &rhs) const;
  bool operator==(const SockAddrV4 &rhs) const;
  std::string to_string() const;
  std::string _ip;
  std::string _port;
};

using ConnectResult = std::expected<Socket, std::error_code>;

coro::Task<ConnectResult> connect(const AddrInfo &ai, std::shared_ptr<Poller> poller);

using BindResult = std::expected<Socket, std::error_code>;

BindResult bind(const AddrInfo &ai);

using ListenResult = std::expected<void, std::error_code>;

ListenResult listen(Socket &skt, int max_connections = MAX_CONNECTIONS);

bool set_keep_alive(int fd, bool keep_alive = true);

const size_t MAX_SINGLE_RECV_SIZE = 1024;

enum class SendError {
  Unknown,
};

std::string_view to_string_view(SendError err);
using SendResult = std::expected<bool, SendError>;
enum class RecvError {
  Unknown,
  Eof,
  NoData,
};
std::string_view to_string_view(RecvError err);
using RecvResult = std::expected<std::string, RecvError>;
SendResult send(int fd, std::string_view data);
RecvResult recv(int fd);
TCP_NAMESPACE_END
#endif  // _XSL_NET_TRANSPORT_UTILS_H_
