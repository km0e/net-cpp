#include "xsl/feature.h"
#include "xsl/wheel/type_traits.h"

void test_origanize_feature_flags() {
  using namespace xsl::wheel;
  using namespace xsl::feature;
  using namespace xsl::wheel::type_traits;
  using Flags1 = _n<node>;
  using FullFlags1 = _n<node>;
  static_assert(std::is_same_v<origanize_feature_flags_t<Flags1, FullFlags1>, _n<node>>);
  using Flags2 = _n<node>;
  using FullFlags2 = _n<>;
  static_assert(std::is_same_v<origanize_feature_flags_t<Flags2, FullFlags2>, _n<>>);
};

int main() {
  test_origanize_feature_flags();
  return 0;
};
