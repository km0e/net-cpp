/**
 * @file test_sys_raw.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief test for compile sys raw
 * @version 0.1
 * @date 2024-08-24
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "xsl/ai.h"
#include <xsl/coro.h>
#include <xsl/logctl.h>
#include <xsl/sys.h>
using namespace xsl::ai;
using namespace xsl::sys;
using namespace xsl::feature;
using namespace xsl;
int main() {
  static_assert(ai::BRL<RawDevice<In<DefaultDeviceTraits>>>);
  static_assert(ai::BWL<RawDevice<Out<DefaultDeviceTraits>>>);
  static_assert(ai::BRL<RawDevice<InOut<DefaultDeviceTraits>>>);
  static_assert(ai::BWL<RawDevice<InOut<DefaultDeviceTraits>>>);
  static_assert(ai::BRWL<RawDevice<InOut<DefaultDeviceTraits>>>);
  return 0;
}
