/**
 * @file sockaddr.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1.0
 * @date 2024-08-20
 *
 * @copyright Copyright (c) 2024
 *
 */
#include <gtest/gtest.h>
#include <xsl/feature.h>
#include <xsl/log.h>
#include <xsl/sys.h>

using namespace xsl::sys::net;
using namespace xsl;

TEST(net, sockaddr_parse) {
  auto res = make_sockaddr<xsl::TcpIpv4>("1.1.1.1", 80);
  std::string ip;
  uint16_t port;
  ASSERT_EQ(res->parse(ip, port), errc{});
  ASSERT_EQ(ip, "1.1.1.1");
  ASSERT_EQ(port, 80);

  auto res2 = make_sockaddr<xsl::TcpIpv6>("ff06:0:0:0:0:0:0:c3", 80);
  std::string ip2;
  uint16_t port2;
  ASSERT_EQ(res2->parse(ip2, port2), errc{});
  ASSERT_EQ(ip2, "ff06::c3");
  ASSERT_EQ(port2, 80);
};

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
