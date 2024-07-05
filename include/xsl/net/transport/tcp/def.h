#pragma once
#ifndef _XSL_NET_TRANSPORT_TCP_DEF_H_
#  define _XSL_NET_TRANSPORT_TCP_DEF_H_

#  define TCP_NAMESPACE_BEGIN namespace xsl::net::transport::tcp {
#  define TCP_NAMESPACE_END }
#  include <unistd.h>
TCP_NAMESPACE_BEGIN
const int MAX_CONNECTIONS = 100000;
const int RECV_TIMEOUT = 1000;
const int KEEP_ALIVE_TIMEOUT_COUNT = 5;
const int ZERO_SIZE_READ_COUNT = 5;

class Socket {
public:
  Socket(int sfd);
  ~Socket();
  int raw_fd() const;

private:
  int sfd;
};

TCP_NAMESPACE_END
#endif
