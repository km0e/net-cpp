#pragma once
#ifndef _XSL_NET_TRANSPORT_TCP_DEF_H_
#  define _XSL_NET_TRANSPORT_TCP_DEF_H_

#  define TCP_NAMESPACE_BEGIN namespace xsl::net::transport::tcp {
#  define TCP_NAMESPACE_END }
TCP_NAMESPACE_BEGIN
const int MAX_CONNECTIONS = 100000;
const int RECV_TIMEOUT = 1000;
const int RECV_RESET_COUNT = 10;
TCP_NAMESPACE_END
#endif
