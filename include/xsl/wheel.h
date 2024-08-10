#pragma once

#ifndef XSL_WHEEL
#  define XSL_WHEEL
#  include "xsl/def.h"
#  include "xsl/wheel/str.h"
#  include "xsl/wheel/utils.h"
XSL_NB
using wheel::bool_from_bytes;
using wheel::bool_to_bytes;
using wheel::FixedString;
using wheel::i32_from_bytes;
using wheel::i32_to_bytes;
using wheel::us_map;
using wheel::dynamic_assert;
XSL_NE
#endif  // XSL_UTILS_WHEEL
