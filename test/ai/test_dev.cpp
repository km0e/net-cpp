#include "xsl/ai.h"
#include "xsl/feature.h"
#include "xsl/logctl.h"

#include <gtest/gtest.h>
using namespace xsl::ai;
struct Tag;
XSL_AI_NB
template <>
struct DeviceTraits<Tag> {
  using value_type = byte;
};
XSL_AI_NE

TEST(Dev, ReadDeviceLike) {
  static_assert(AsyncReadDeviceLike<AsyncDevice<xsl::feature::In<Tag>>, xsl::byte>);
  static_assert(AsyncWriteDeviceLike<AsyncDevice<xsl::feature::Out<Tag>>, xsl::byte>);
  static_assert(AsyncReadDeviceLike<AsyncDevice<xsl::feature::InOut<Tag>>, xsl::byte>);
  static_assert(AsyncWriteDeviceLike<AsyncDevice<xsl::feature::InOut<Tag>>, xsl::byte>);
  static_assert(AsyncReadWriteDeviceLike<AsyncDevice<xsl::feature::InOut<Tag>>, xsl::byte>);
}

int main(int argc, char **argv) {
  xsl::no_log();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
