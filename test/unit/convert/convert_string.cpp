/**
 * @file test_convert_string.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-09-01
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "xsl/convert.h"

#include <gtest/gtest.h>

#include <string>

struct A {
  std::string to_string() const { return "A"; }
  std::string_view to_string_view() const { return "A"; }
};
struct B {
  operator std::string() const { return "B"; }
  operator std::string_view() const { return "B"; }
};

TEST(convert, string) {
  A a;
  B b;
  std::string c{"C"};
  ASSERT_EQ("A", xsl::to_string(a));
  ASSERT_EQ("A", xsl::to_string_view(a));
  ASSERT_EQ("B", xsl::to_string(b));
  ASSERT_EQ("B", xsl::to_string_view(b));
  ASSERT_EQ("C", xsl::to_string(c));
  ASSERT_EQ("C", xsl::to_string_view(c));
};

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
