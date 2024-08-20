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
#include "xsl/logctl.h"
#include "xsl/sys.h"

#include <gtest/gtest.h>

using namespace xsl::sys::net;
TEST(net, sockaddr_zero) {
  SockAddr<> zero{};
  zero.reset();
  SockAddr<> init{};
  ASSERT_EQ(zero, init);
};

int main(int argc, char **argv) {
  xsl::no_log();
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
