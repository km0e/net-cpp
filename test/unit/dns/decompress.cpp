/**
 * @file decompress.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief DNS decompression test
 * @version 0.1.0
 * @date 2024-09-09
 *
 * @copyright Copyright (c) 2024
 *
 */
#include <gtest/gtest.h>
#include <xsl/def.h>
#include <xsl/net.h>

using namespace xsl::dns;
using namespace xsl;
TEST(dns, decompress) {
  char base[512]
      = {"\3www\6google\3com\0"
         "\3abc\xc0\4"};
  auto name_span = std::as_bytes(std::span{base + 0, 512});
  DnDecompressor decompressor(reinterpret_cast<byte *>(base));
  ASSERT_EQ(decompressor.decompress(name_span), errc{});
  ASSERT_EQ(decompressor.dn(), "www.google.com.");
  ASSERT_EQ(decompressor.decompress(name_span), errc{});
  ASSERT_EQ(decompressor.dn(), "abc.google.com.");
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
