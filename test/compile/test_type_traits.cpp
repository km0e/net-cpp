/**
 * @file test_type_traits.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-08-25
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "xsl/type_traits.h"

#include <cstddef>
using namespace xsl;

template <std::size_t N>
struct test {};

void _is_same_pack() {
  static_assert(is_same_pack_v<_n<int, char>, _n<int, char>>);
  static_assert(is_same_pack_v<_n<int, char>, _n<int, char, char>>);
  static_assert(is_same_pack_v<std::integral_constant<int, 1>, std::integral_constant<char, 'a'>>);
  static_assert(
      is_same_pack_v<std::integer_sequence<int, 1>, std::integer_sequence<char, 'a', 'b'>>);
  static_assert(is_same_pack_v<test<1>, test<1>>);

  static_assert(!is_same_pack_v<_n<int, char>, _2<char, int>>);
}

int main() {
  _is_same_pack();
  return 0;
}
