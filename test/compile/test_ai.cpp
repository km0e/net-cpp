/**
 * @file test_ai.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief test for some concepts in xsl/ai.h
 * @version 0.1
 * @date 2024-08-18
 *
 * @copyright Copyright (c) 2024
 *
 */
#include <xsl/coro.h>
#include <xsl/logctl.h>
#include <xsl/net.h>
using namespace xsl::ai;
using namespace xsl::sys;
using namespace xsl::feature;
using namespace xsl;
int main() {
  static_assert(AsyncReadDeviceLike<AsyncRawDevice<In<DefaultDeviceTraits>>, byte>);
  static_assert(AsyncWriteDeviceLike<AsyncRawDevice<Out<DefaultDeviceTraits>>, byte>);
  static_assert(AsyncReadDeviceLike<AsyncRawDevice<InOut<DefaultDeviceTraits>>, byte>);
  static_assert(AsyncWriteDeviceLike<AsyncRawDevice<InOut<DefaultDeviceTraits>>, byte>);
  static_assert(AsyncReadWriteDeviceLike<AsyncRawDevice<InOut<DefaultDeviceTraits>>, byte>);
  return 0;
}
