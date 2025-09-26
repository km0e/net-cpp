/**
 * @file rc.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1
 * @date 2025-09-15
 *
 * @copyright Copyright (c) 2025
 *
 */
#include <gtest/gtest.h>
#include <xsl/wheel/rc.h>

using namespace xsl::wheel;

TEST(Rc, Basic) {
  Rc<int> rc1(42);
  EXPECT_EQ(*rc1, 42);

  Rc<int> rc2 = rc1;  // Copy constructor
  EXPECT_EQ(*rc2, 42);

  Rc<int> rc3 = std::move(rc1);  // Move constructor
  EXPECT_EQ(*rc3, 42);

  Rc<int> rc4(100);
  rc4 = rc2;  // Copy assignment
  EXPECT_EQ(*rc4, 42);

  Rc<int> rc5(200);
  rc5 = std::move(rc2);  // Move assignment
  EXPECT_EQ(*rc5, 42);
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
