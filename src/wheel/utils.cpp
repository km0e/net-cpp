#include "xsl/wheel/def.h"
#include "xsl/wheel/utils.h"

XSL_WHEEL_NB

void dynamic_assert(bool cond, std::string_view msg, std::source_location loc) {
  dynamic_assert<std::string_view>(cond, msg, loc);
}
XSL_WHEEL_NE
