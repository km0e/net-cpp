/**
 * @file test_decompress.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief DNS decompression test
 * @version 0.1
 * @date 2024-09-09
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "xsl/def.h"
#include "xsl/logctl.h"
#include "xsl/net.h"
#include "xsl/wheel.h"

#include <gtest/gtest.h>

#include <string_view>
using namespace xsl::dns;
using namespace xsl;
TEST(dns, decompress) {
  xsl::byte base[512]
      = {"\3www\6google\3com\0"
         "\3abc\xc0\4"};
  xsl::byte buf[512];
  auto name_span = xsl::as_bytes(std::span{base + 0, 512});
  auto buf_span = xsl::as_writable_bytes(std::span{buf, 512});
  DnDecompressor decompressor(base);
  ASSERT_EQ(decompressor.prepare(name_span), errc{});
  decompressor.decompress(buf_span);
  DEBUG("{}", std::string_view(reinterpret_cast<char *>(buf), 15));
  ASSERT_EQ(std::string_view(reinterpret_cast<char *>(buf), 15), "www.google.com.");
  ASSERT_EQ(decompressor.prepare(name_span), errc{});
  buf_span = xsl::as_writable_bytes(std::span{buf, 512});
  decompressor.decompress(buf_span);
  DEBUG("{}", std::string_view(reinterpret_cast<char *>(buf), 15));
  ASSERT_EQ(std::string_view(reinterpret_cast<char *>(buf), 15), "abc.google.com.");
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
