/**
 * @file test_socket.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief test for compile socket
 * @version 0.1
 * @date 2024-08-31
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "xsl/feature.h"
#include "xsl/io/byte.h"
#include "xsl/sys.h"
using namespace xsl::sys;
using namespace xsl;
int main() {
  static_assert(io::BRL<net::Socket<TcpIp>>);
  static_assert(io::BWL<net::Socket<TcpIp>>);
  static_assert(io::BRWL<net::Socket<TcpIp>>);

  static_assert(io::ABILike<net::AsyncSocket<TcpIp>>);
  static_assert(io::ABOLike<net::AsyncSocket<TcpIp>>);
  static_assert(io::ABIOLike<net::AsyncSocket<TcpIp>>);
  return 0;
}
