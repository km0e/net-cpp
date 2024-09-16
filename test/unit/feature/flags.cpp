/**
 * @file test_flags.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "xsl/feature.h"

#include <gtest/gtest.h>
using namespace xsl;
template <class... Flags>
struct Test;

template <>
struct Test<Placeholder, float> {
  static int f() { return 1; }
};

template <>
struct Test<int, float> {
  static int f() { return 2; }
};

template <>
struct Test<int, Placeholder> {
  static int f() { return 3; }
};

template <>
struct Test<Placeholder, char> {
  static int f() { return 4; }
};

template <class... Flags>
using Test_t = organize_feature_flags_t<Test<int, set<char, float>>, Flags...>;

TEST(feature, organize_feature_flags_t) {
  ASSERT_EQ((Test_t<float>::f()), 1);
  ASSERT_EQ((Test_t<int, float>::f()), 2);
  ASSERT_EQ((Test_t<int>::f()), 3);
  ASSERT_EQ((Test_t<char>::f()), 4);
};

int main() {
  ::testing::InitGoogleTest();
  return RUN_ALL_TESTS();
};
