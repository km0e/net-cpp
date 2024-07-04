#include "xsl/convert.h"
#include "xsl/logctl.h"

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
  EXPECT_EQ("A", xsl::to_string(a));
  EXPECT_EQ("A", xsl::to_string_view(a));
  EXPECT_EQ("B", xsl::to_string(b));
  EXPECT_EQ("B", xsl::to_string_view(b));
  EXPECT_EQ("C", xsl::to_string(c));
  EXPECT_EQ("C", xsl::to_string_view(c));
};

int main(int argc, char **argv) {
  xsl::no_log();
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
