#pragma once
#ifndef XSL_NET_TRANSPORT_TCP_DEF
#  define XSL_NET_TRANSPORT_TCP_DEF

#  define TCP_NB namespace xsl::net::transport::tcp {
#  define TCP_NE }
#  include <unistd.h>
TCP_NB
const int MAX_CONNECTIONS = 100000;
const int RECV_TIMEOUT = 1000;
const int KEEP_ALIVE_TIMEOUT_COUNT = 5;
const int ZERO_SIZE_READ_COUNT = 5;
const size_t MAX_SINGLE_RECV_SIZE = 1024;

TCP_NE
#endif
