#pragma once
#ifndef _XSL_NET_TRANSPORT_UTILS_H_
#  define _XSL_NET_TRANSPORT_UTILS_H_
#  include "xsl/net/transport/tcp/def.h"
#  include "xsl/wheel.h"
TCP_NAMESPACE_BEGIN
class TcpClientSockConfig {
public:
  bool keep_alive = false;
  bool non_blocking = false;
};

int create_tcp_client(const char *ip, const char *port, TcpClientSockConfig config = {});
class TcpServerSockConfig {
public:
  int max_connections = MAX_CONNECTIONS;
  bool keep_alive = false;
  bool non_blocking = false;
};
int create_tcp_server(const char *host, int port, TcpServerSockConfig config = {});
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
};
string_view to_string(RecvError err);
using RecvResult = Result<string, RecvError>;
SendResult send(int fd, string_view data);
RecvResult recv(int fd);
TCP_NAMESPACE_END
#endif  // _XSL_NET_TRANSPORT_UTILS_H_
