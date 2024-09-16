/**
 * @file test_ser.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Test serialization and deserialization utilities
 * @version 0.1
 * @date 2024-09-11
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "xsl/def.h"
#include "xsl/ser.h"
#include "xsl/wheel.h"

#include <gtest/gtest.h>

#include <span>

using namespace xsl;

TEST(ser, i32) {
  int32_t value = 0x12345678, result;
  byte bytes[4];
  byte expected[4] = {0x78, 0x56, 0x34, 0x12};

  serialize(bytes, value);
  EXPECT_EQ(std::memcmp(bytes, expected, 4), 0);

  deserialize(bytes, result);
  EXPECT_EQ(value, result);

  std::memset(bytes, 0, 4);
  std::span<byte> buf(bytes, 4);

  serialized(buf, value);
  EXPECT_EQ(0, buf.size());
  EXPECT_EQ(std::memcmp(bytes, expected, 4), 0);

  result = 0;

  auto c_bytes = xsl::as_bytes(bytes, 4);
  deserialized(c_bytes, result);
  EXPECT_EQ(value, result);
}

TEST(str, u16) {
  uint16_t value = 0x1234, result;
  byte bytes[2];
  byte expected[2] = {0x34, 0x12};

  serialize(bytes, value);
  EXPECT_EQ(std::memcmp(bytes, expected, 2), 0);

  deserialize(bytes, result);
  EXPECT_EQ(value, result);

  std::memset(bytes, 0, 2);
  std::span<byte> buf(bytes, 2);

  serialized(buf, value);
  EXPECT_EQ(0, buf.size());
  EXPECT_EQ(std::memcmp(bytes, expected, 2), 0);

  result = 0;

  auto c_bytes = xsl::as_bytes(bytes, 2);
  deserialized(c_bytes, result);
  EXPECT_EQ(value, result);
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
