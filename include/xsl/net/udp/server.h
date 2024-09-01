/**
 * @file server.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.11
 * @date 2024-08-19
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_NET_UDP_SERVER
#  define XSL_NET_UDP_SERVER
#  include "xsl/net/udp/def.h"

#  include <expected>
XSL_UDP_NB
/**
 * @brief TcpServer
 *
 * @tparam LowerLayer, such as Ip<Version>(Version = 4 or 6)
 */
template <class LowerLayer>
class Server;  // TODO: Implement Server

XSL_UDP_NE
#endif
