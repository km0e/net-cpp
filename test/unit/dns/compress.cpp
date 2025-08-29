/**
 * @file compress.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief DNS compression test
 * @version 0.1.0
 * @date 2024-08-20
 *
 * @copyright Copyright (c) 2024
 *
 */
#include <gtest/gtest.h>
#include <xsl/log.h>
#include <xsl/net.h>

#include <string_view>
using namespace xsl::dns;

class DnCompressCommonTest : public ::testing::Test {
protected:
  DnCompressCommonTest() {}

  void SetUp() override {}

  void TearDown() override {}

  ~DnCompressCommonTest() {}

  void compress(std::string_view src_dns[], uint8_t *base, std::size_t off) {
    uint8_t *dst_dns = base + off;
    xsl::byte *ptr = reinterpret_cast<xsl::byte *>(dst_dns);
    memset(dst_dns, 0, 256);
    DnCompressor dc(reinterpret_cast<std::byte *>(base));
    auto res_n = dc.prepare(src_dns[0]);
    ASSERT_EQ(*res_n, 16);
    dc.compress(ptr);
    ptr += *res_n;
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
    dc.compress(ptr);
    ptr += *res_n;
    ASSERT_EQ(dst_dns[16], 4);
    ASSERT_EQ(dst_dns[17], 'm');
    ASSERT_EQ(dst_dns[18], 'a');
    ASSERT_EQ(dst_dns[19], 'i');
    ASSERT_EQ(dst_dns[20], 'l');
    ASSERT_EQ(dst_dns[21], 0xc0 + (off >> 8));    // pointer to the first domain name
    ASSERT_EQ(dst_dns[22], 0x04 + (off & 0xff));  // pointer to the first domain name
    res_n = dc.prepare(src_dns[2]);
    ASSERT_EQ(*res_n, 2);
    dc.compress(ptr);
    ptr += *res_n;
    ASSERT_EQ(dst_dns[23], 0xc0 + (off >> 8));    // pointer to the first domain name
    ASSERT_EQ(dst_dns[24], 0x0b + (off & 0xff));  // pointer to the first domain name
    res_n = dc.prepare(src_dns[3]);
    ASSERT_EQ(*res_n, 1);
    dc.compress(ptr);
    ptr += *res_n;
    ASSERT_EQ(dst_dns[25], 0);
    ASSERT_EQ(reinterpret_cast<uint8_t *>(ptr) - dst_dns, 26);
  }
};
TEST_F(DnCompressCommonTest, SameBase) {
  std::string_view src_dns[] = {"www.google.com.", "mail.google.com", "com", "."};
  uint8_t dst_dns[256];
  compress(src_dns, dst_dns, 0);
}
TEST_F(DnCompressCommonTest, DifferentBase) {
  std::string_view src_dns[] = {"www.google.com.", "mail.google.com", "com", "."};
  uint8_t base[256];
  compress(src_dns, base, 4);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
