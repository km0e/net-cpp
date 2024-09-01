/**
 * @file test_type_traits.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.11
 * @date 2024-08-25
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "xsl/type_traits.h"

#include <cstddef>
#include <type_traits>
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

template <class T>
struct test2 {
  using type = std::make_unsigned_t<T>;
};
void _for_each() {
  static_assert(std::is_same_v<for_each_t<test2, _n<int, char>>, _n<unsigned, unsigned char>>);
}

template <class L, class R>
struct test3 : std::bool_constant<std::is_same_v<L, R>> {};

void _remove_first_if() {
  static_assert(std::is_same_v<remove_first_if<test3, int, _n<int, char>>, _2<_n<char>, int>>);
  static_assert(std::is_same_v<remove_first_if<test3, int, _n<char, int>>, _2<_n<char>, int>>);
  static_assert(
      std::is_same_v<remove_first_if<test3, int, _n<char, char>>, _2<_n<char, char>, void>>);
}

int main() {
  _is_same_pack();
  return 0;
}
