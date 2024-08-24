#include "xsl/feature.h"
#include <gtest/gtest.h>

using namespace xsl::feature;
template <class... Flags>
struct Test;

template <>
struct Test<placeholder, float> {
  static int f() { return 1; }
};

template <>
struct Test<int, float> {
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
using Test_t = organize_feature_flags_t<Test<int, set<char, float>>, Flags...>;

TEST(feature, organize_feature_flags_t) {
  ASSERT_EQ((Test_t<float>::f()), 1);
  ASSERT_EQ((Test_t<int,float>::f()), 2);
  ASSERT_EQ((Test_t<int>::f()), 3);
  ASSERT_EQ((Test_t<char>::f()), 4);
};

int main() {
  ::testing::InitGoogleTest();
  return RUN_ALL_TESTS();
};
