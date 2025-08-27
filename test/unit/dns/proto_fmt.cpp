/**
 * @file proto_fmt.cpp
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
#include <xsl/ser.h>

#include <cstdint>
#include <format>

using namespace xsl;
using namespace xsl::dns;

TEST(dns_proto, type) {
  auto cases = {
      Type::A,  Type::NS,  Type::MD,    Type::MF,    Type::CNAME, Type::SOA,   Type::MB,
      Type::MG, Type::MR,  Type::_NULL, Type::WKS,   Type::PTR,   Type::HINFO, Type::MINFO,
      Type::MX, Type::TXT, Type::AXFR,  Type::MAILB, Type::MAILA, Type::ANY,
  };
  for (auto t : cases) {
    Type type = t;
    EXPECT_EQ(type.to_string_view(), std::format("{}", type));
  }
}

TEST(dns_proto, class) {
  auto cases = {
      Class::IN, Class::CS, Class::CH, Class::HS, Class::ANY,
  };
  for (auto c : cases) {
    Class class_ = c;
    EXPECT_EQ(class_.to_string_view(), std::format("{}", class_));
  }
}

TEST(dns_proto, rcode) {
  auto cases = {
      RCode::NO_ERROR,   RCode::FORMAT_ERROR,    RCode::SERVER_FAILURE,
      RCode::NAME_ERROR, RCode::NOT_IMPLEMENTED, RCode::REFUSED,
  };
  for (auto c : cases) {
    RCode rcode = c;
    EXPECT_EQ(rcode.to_string_view(), std::format("{}", rcode));
  }
}

TEST(dns_proto, header) {
  Header header = {.id = 0x1234,
                   .flags = 0x1234,
                   .qdcount = 0x1234,
                   .ancount = 0x1234,
                   .nscount = 0x1234,
                   .arcount = 0x1234};

  uint8_t bytes[12];
  uint8_t expected[12] = {0x34, 0x12, 0x12, 0x34, 0x12, 0x34, 0x12, 0x34, 0x12, 0x34, 0x12, 0x34};
  std::span<byte> buf = std::as_writable_bytes(std::span(bytes, 12));

  header.serialize(buf);
  for (auto i = 0uz; i < 12; ++i) {
    EXPECT_EQ(bytes[i], expected[i]) << "Byte " << i << " mismatch";
  }

  Header result = {.id = 0, .flags = 0, .qdcount = 0, .ancount = 0, .nscount = 0, .arcount = 0};

  std::span<const byte> bytes_span = std::as_bytes(std::span(bytes, 12));
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
  uint8_t expected[256]
      = {3, 'w', 'w', 'w', 6,   'g', 'o',  'o',  'g', 'l', 'e', 3, 'c',  'o',  'm', 0, 0, 1, 0,
         1, 4,   'm', 'a', 'i', 'l', 0xc0, 0x04, 0,   1,   0,   1, 0xc0, 0x04, 0,   1, 0, 1};

  byte bytes[256];
  std::span<byte> buf(bytes, 256);

  DnCompressor compressor{bytes};
  Type type = Type::A;
  Class class_ = Class::IN;

  auto sz = compressor.prepare(dns[0]);
  compressor.compress(buf.data());
  buf = buf.subspan(sz.value());
  xsl::serialized(buf, type, class_);
  EXPECT_EQ(std::memcmp(bytes, expected, 18), 0);

  std::span<const byte> bytes_span(bytes, 256);
  auto status = dns::skip_question(bytes_span);
  EXPECT_EQ(status, errc{});
  EXPECT_EQ(256 - bytes_span.size(), 20);

  sz = compressor.prepare(dns[1]);
  compressor.compress(buf.data());
  buf = buf.subspan(sz.value());
  xsl::serialized(buf, type, class_);
  EXPECT_EQ(std::memcmp(bytes + 20, expected + 20, 11), 0);

  status = dns::skip_question(bytes_span);
  EXPECT_EQ(status, errc{});
  EXPECT_EQ(256 - bytes_span.size(), 31);

  sz = compressor.prepare(dns[2]);
  compressor.compress(buf.data());
  buf = buf.subspan(sz.value());
  xsl::serialized(buf, type, class_);
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
