#include "xsl/feature.h"
#include <gtest/gtest.h>

using namespace xsl::feature;
template <class... Flags>
struct Test;

template <>
struct Test<placeholder, int> {
  static int f() { return 1; }
};

template <>
struct Test<int, int> {
  static int f() { return 2; }
};

template <>
struct Test<int, placeholder> {
  static int f() { return 3; }
};

template <>
struct Test<placeholder, char> {
  static int f() { return 4; }
};

template <class... Flags>
using Test_t = origanize_feature_flags_t<Test<int, set<char, int>>, Flags...>;

TEST(feature, origanize_feature_flags_t) {
  ASSERT_EQ((Test_t<placeholder, int>::f()), 1);
  ASSERT_EQ((Test_t<int>::f()), 3);
  ASSERT_EQ((Test_t<char>::f()), 4);
};

int main() {
  ::testing::InitGoogleTest();
  return RUN_ALL_TESTS();
};
