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
#include "xsl/net.h"

#include <gtest/gtest.h>

using namespace xsl;
using namespace xsl::dns;

TEST(dns_proto, type) {
  Type type = Type::A;
  byte bytes[2];
  byte expected[2] = {0, 1};
  std::span<byte> buf(bytes, 2);

  serialized(buf, type);
  EXPECT_EQ(std::memcmp(bytes, expected, 2), 0);

  Type result = Type::ANY;

  deserialize(bytes, result);
  EXPECT_EQ(type, result);
}

TEST(dns_proto, class) {
  Class class_ = Class::IN;
  byte bytes[2];
  byte expected[2] = {0, 1};
  std::span<byte> buf(bytes, 2);

  serialized(buf, class_);
  EXPECT_EQ(std::memcmp(bytes, expected, 2), 0);

  Class result = Class::ANY;

  deserialize(bytes, result);
  EXPECT_EQ(class_, result);
}

TEST(dns_proto, header) {
  Header header = {.id = 0x1234,
                   .flags = 0x1234,
                   .qdcount = 0x1234,
                   .ancount = 0x1234,
                   .nscount = 0x1234,
                   .arcount = 0x1234};

  byte bytes[12];
  byte expected[12] = {0x34, 0x12, 0x12, 0x34, 0x12, 0x34, 0x12, 0x34, 0x12, 0x34, 0x12, 0x34};
  std::span<byte> buf(bytes, 12);

  header.serialize(buf);
  EXPECT_EQ(std::memcmp(bytes, expected, 12), 0);

  Header result = {.id = 0, .flags = 0, .qdcount = 0, .ancount = 0, .nscount = 0, .arcount = 0};

  std::span<const byte> bytes_span(bytes, 12);
  result.deserialize(bytes_span);
  ASSERT_EQ(bytes_span.size(), 0);
  EXPECT_EQ(header.id, result.id);
  EXPECT_EQ(header.flags, result.flags);
  EXPECT_EQ(header.qdcount, result.qdcount);
  EXPECT_EQ(header.ancount, result.ancount);
  EXPECT_EQ(header.nscount, result.nscount);
  EXPECT_EQ(header.arcount, result.arcount);
}

TEST(dns_proto, question) {
  std::string_view dns[] = {"www.google.com", "mail.google.com.", "google.com"};
  byte expected[256]
      = {3, 'w', 'w', 'w', 6,   'g', 'o',  'o',  'g', 'l', 'e', 3, 'c',  'o',  'm', 0, 0, 1, 0,
         1, 4,   'm', 'a', 'i', 'l', 0xc0, 0x04, 0,   1,   0,   1, 0xc0, 0x04, 0,   1, 0, 1};

  byte bytes[256];
  std::span<byte> buf(bytes, 256);

  DnCompressor compressor{bytes};
  Type type = Type::A;
  Class class_ = Class::IN;

  auto status = dns::serialized(buf, dns[0], type, class_, compressor);
  EXPECT_EQ(status, errc{});
  EXPECT_EQ(std::memcmp(bytes, expected, 18), 0);

  std::span<const byte> bytes_span(bytes, 256);
  status = dns::skip_question(bytes_span);
  EXPECT_EQ(status, errc{});
  EXPECT_EQ(256 - bytes_span.size(), 20);

  status = dns::serialized(buf, dns[1], type, class_, compressor);
  EXPECT_EQ(status, errc{});
  EXPECT_EQ(std::memcmp(bytes + 20, expected + 20, 11), 0);

  status = dns::skip_question(bytes_span);
  EXPECT_EQ(status, errc{});
  EXPECT_EQ(256 - bytes_span.size(), 31);

  status = dns::serialized(buf, dns[2], type, class_, compressor);
  EXPECT_EQ(status, errc{});
  EXPECT_EQ(std::memcmp(bytes + 31, expected + 31, 5), 0);

  status = dns::skip_question(bytes_span);
  EXPECT_EQ(status, errc{});
  EXPECT_EQ(256 - bytes_span.size(), 37);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
