/**
 * @file test_sys_raw.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief test for compile sys raw
 * @version 0.11
 * @date 2024-08-24
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "xsl/io/byte.h"
#include "xsl/sys.h"
using namespace xsl::sys;
using namespace xsl;
int main() {
  static_assert(io::BRL<RawDevice<In<byte>>>);
  static_assert(io::BWL<RawDevice<Out<byte>>>);
  static_assert(io::BRL<RawDevice<InOut<byte>>>);
  static_assert(io::BWL<RawDevice<InOut<byte>>>);
  static_assert(io::BRWL<RawDevice<InOut<byte>>>);

  static_assert(io::ABILike<RawAsyncDevice<In<byte>>>);
  static_assert(io::ABOLike<RawAsyncDevice<Out<byte>>>);
  static_assert(io::ABILike<RawAsyncDevice<InOut<byte>>>);
  static_assert(io::ABOLike<RawAsyncDevice<InOut<byte>>>);
  static_assert(io::ABIOLike<RawAsyncDevice<InOut<byte>>>);
  return 0;
}
