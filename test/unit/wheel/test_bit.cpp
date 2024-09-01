/**
 * @file test_bit.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-09-01
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "xsl/wheel/bit.h"

#include <gtest/gtest.h>

using namespace xsl::wheel;

TEST(bit, ceil2pow2) {
  EXPECT_EQ(ceil2pow2(0), 1);
  EXPECT_EQ(ceil2pow2(1), 1);
  EXPECT_EQ(ceil2pow2(2), 2);
  EXPECT_EQ(ceil2pow2(3), 4);
  EXPECT_EQ(ceil2pow2(5), 8);
  EXPECT_EQ(ceil2pow2(9), 16);
  EXPECT_EQ(ceil2pow2(15), 16);
  EXPECT_EQ(ceil2pow2(17), 32);
  EXPECT_EQ(ceil2pow2(31), 32);
  EXPECT_EQ(ceil2pow2(32), 32);
  EXPECT_EQ(ceil2pow2(33), 64);
}
int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
