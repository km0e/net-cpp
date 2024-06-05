#pragma once
#ifndef _XSL_NET_TRANSPORT_UTILS_H_
#  define _XSL_NET_TRANSPORT_UTILS_H_
#  include "xsl/net/transport/tcp/def.h"
#  include "xsl/wheel.h"
TCP_NAMESPACE_BEGIN
class SockAddrV4View {
public:
  SockAddrV4View() = default;
  SockAddrV4View(const char *ip, const char *port);
  //eg: "0.0.0.1:80"
  SockAddrV4View(const char *sa4);
  SockAddrV4View(string_view sa4);
  SockAddrV4View(string_view ip, string_view port);
  bool operator==(const SockAddrV4View &rhs) const;
  const char *ip() const;
  const char *port() const;
  string to_string() const;
  string_view _ip;
  string_view _port;
};

class SockAddrV4 {
public:
  SockAddrV4(int fd);
  SockAddrV4(const char *ip, const char *port);
  SockAddrV4(string_view ip, string_view port);
  SockAddrV4(SockAddrV4View sa4);
  SockAddrV4View view() const;
  bool operator==(const SockAddrV4View &rhs) const;
  bool operator==(const SockAddrV4 &rhs) const;
  string to_string() const;
  string _ip;
  string _port;
};

class TcpClientSockConfig {
public:
  bool keep_alive = false;
  bool non_blocking = false;
};

int create_tcp_client(const char *ip, const char *port, TcpClientSockConfig config = {});
int create_tcp_client(SockAddrV4View sa4, TcpClientSockConfig config = {});
class TcpServerSockConfig {
public:
  int max_connections = MAX_CONNECTIONS;
  bool keep_alive = false;
  bool non_blocking = false;
  bool reuse_addr = true;
};
int create_tcp_server(const char *ip, int port, TcpServerSockConfig config = {});
int create_tcp_server(SockAddrV4View sa4, TcpServerSockConfig config = {});

bool set_keep_alive(int fd, bool keep_alive = true);

const size_t MAX_SINGLE_RECV_SIZE = 1024;

enum class SendError {
  Unknown,
};

string_view to_string(SendError err);
using SendResult = Result<size_t, SendError>;
enum class RecvError {
  Unknown,
  Eof,
  NoData,
};
string_view to_string(RecvError err);
using RecvResult = Result<string, RecvError>;
SendResult send(int fd, string_view data);
RecvResult recv(int fd);
TCP_NAMESPACE_END
#endif  // _XSL_NET_TRANSPORT_UTILS_H_
