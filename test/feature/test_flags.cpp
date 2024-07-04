#include "xsl/feature.h"
#include "xsl/logctl.h"
#include "xsl/wheel/type_traits.h"
#include <gtest/gtest.h>

TEST(type_traits, origanize_feature_flags_t) {
  using namespace xsl::wheel;
  using namespace xsl::feature;
  using namespace xsl::wheel::type_traits;
  using Flags1 = _n<node>;
  using FullFlags1 = _n<node>;
  ASSERT_TRUE((std::is_same_v<origanize_feature_flags_t<Flags1, FullFlags1>, _n<node>>));
  using Flags2 = _n<node>;
  using FullFlags2 = _n<>;
  ASSERT_TRUE((std::is_same_v<origanize_feature_flags_t<Flags2, FullFlags2>, _n<>>));
};

TEST(log, log){
  DEBUG("err");
  INFO("ok");
  WARNING("ok");
  xsl::set_log_level(xsl::LogLevel::DEBUG);
  ERROR("{}",QUILL_COMPILE_ACTIVE_LOG_LEVEL);
  DEBUG("err");
  INFO("ok");
  WARNING("ok");
  xsl::set_log_level(xsl::LogLevel::WARNING);
  DEBUG("err");
  INFO("err");
  WARNING("ok");
  ASSERT_TRUE(false);
}


int main() {
  ::testing::InitGoogleTest();
  return RUN_ALL_TESTS();
};
