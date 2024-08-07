#include "xsl/logctl.h"
#include "xsl/wheel/str.h"

#include <gtest/gtest.h>

using namespace xsl::wheel;

TEST(str, i32_to_bytes) {
  int32_t value = 0x12345678;
  std::byte bytes[4];
  i32_to_bytes(value, bytes);
  ASSERT_EQ(value, i32_from_bytes(bytes));
}

int main(int argc, char **argv) {
  xsl::no_log();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
