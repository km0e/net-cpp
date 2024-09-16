/**
 * @file test_net.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-08-20
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "xsl/feature.h"
#include "xsl/logctl.h"
#include "xsl/sys.h"

#include <gtest/gtest.h>

using namespace xsl::sys::net;
using namespace xsl;

TEST(net, sockaddr_zero) {
  SockAddrCompose<xsl::TcpIpv4> zero{};
  zero.reset();
  SockAddrCompose<xsl::TcpIpv4> init{};
  ASSERT_EQ(zero, init);
};

TEST(net, sockaddr_parse) {
  SockAddrCompose<xsl::TcpIpv4> addr{"1.1.1.1", 80};
  std::string ip;
  uint16_t port;
  ASSERT_EQ(addr.parse(ip, port), errc{});
  ASSERT_EQ(ip, "1.1.1.1");
  ASSERT_EQ(port, 80);

  SockAddrCompose<xsl::TcpIpv6> addr2{"ff06:0:0:0:0:0:0:c3", 80};
  std::string ip2;
  uint16_t port2;
  ASSERT_EQ(addr2.parse(ip2, port2), errc{});
  ASSERT_EQ(ip2, "ff06::c3");
  ASSERT_EQ(port2, 80);
};

int main(int argc, char **argv) {
  xsl::no_log();
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
