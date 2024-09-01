#include "xsl/def.h"
#include "xsl/wheel.h"

#include <gtest/gtest.h>

using namespace xsl;

TEST(str, i32_bytes) {
  int32_t value = 0x12345678;
  xsl::byte bytes[4];
  i32_to_bytes(value, bytes);
  ASSERT_EQ(value, i32_from_bytes(bytes));
}

TEST(str, u16_bytes) {
  uint16_t value = 0x1234;
  xsl::byte bytes[2];
  u16_to_bytes(value, bytes);
  ASSERT_EQ(value, u16_from_bytes(bytes));
}



int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
