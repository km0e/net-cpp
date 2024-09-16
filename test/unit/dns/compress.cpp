/**
 * @file test_compress.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief DNS compression test
 * @version 0.1
 * @date 2024-08-20
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "xsl/logctl.h"
#include "xsl/net.h"

#include <gtest/gtest.h>

#include <string_view>
using namespace xsl::dns;
TEST(dns, compress) {
  std::string_view src_dns[] = {"www.google.com.", "mail.google.com", "com", "."};
  unsigned char dst_dns[256];
  auto data_span = std::span{dst_dns, 256};
  memset(dst_dns, 0, 256);
  DnCompressor dc(dst_dns);
  auto res_n = dc.prepare(src_dns[0]);
  ASSERT_EQ(*res_n, 16);
  dc.compress(data_span);
  ASSERT_EQ(dst_dns[0], 3);
  ASSERT_EQ(dst_dns[1], 'w');
  ASSERT_EQ(dst_dns[2], 'w');
  ASSERT_EQ(dst_dns[3], 'w');
  ASSERT_EQ(dst_dns[4], 6);
  ASSERT_EQ(dst_dns[5], 'g');
  ASSERT_EQ(dst_dns[6], 'o');
  ASSERT_EQ(dst_dns[7], 'o');
  ASSERT_EQ(dst_dns[8], 'g');
  ASSERT_EQ(dst_dns[9], 'l');
  ASSERT_EQ(dst_dns[10], 'e');
  ASSERT_EQ(dst_dns[11], 3);
  ASSERT_EQ(dst_dns[12], 'c');
  ASSERT_EQ(dst_dns[13], 'o');
  ASSERT_EQ(dst_dns[14], 'm');
  ASSERT_EQ(dst_dns[15], 0);
  res_n = dc.prepare(src_dns[1]);
  ASSERT_EQ(*res_n, 7);
  dc.compress(data_span);
  ASSERT_EQ(dst_dns[16], 4);
  ASSERT_EQ(dst_dns[17], 'm');
  ASSERT_EQ(dst_dns[18], 'a');
  ASSERT_EQ(dst_dns[19], 'i');
  ASSERT_EQ(dst_dns[20], 'l');
  ASSERT_EQ(dst_dns[21], 0xc0);
  ASSERT_EQ(dst_dns[22], 0x04);
  res_n = dc.prepare(src_dns[2]);
  ASSERT_EQ(*res_n, 2);
  dc.compress(data_span);
  ASSERT_EQ(dst_dns[23], 0xc0);
  ASSERT_EQ(dst_dns[24], 0x0b);
  res_n = dc.prepare(src_dns[3]);
  ASSERT_EQ(*res_n, 1);
  dc.compress(data_span);
  ASSERT_EQ(dst_dns[25], 0);
  ASSERT_EQ(256 - data_span.size(), 26);
};

int main(int argc, char **argv) {
  xsl::no_log();
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
