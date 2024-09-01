/**
 * @file test_utils.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-09-01
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "xsl/wheel/utils.h"

#include <gtest/gtest.h>

using namespace xsl::wheel;

TEST(utils, defer) {
  int i = 0;
  {
    Defer defer([&i]() { i = 1; });
    ASSERT_EQ(i, 0);
  }
  ASSERT_EQ(i, 1);
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
