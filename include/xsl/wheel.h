#pragma once

#ifndef XSL_WHEEL
#  define XSL_WHEEL
#  include "xsl/def.h"
#  include "xsl/wheel/ptr.h"
#  include "xsl/wheel/str.h"
#  include "xsl/wheel/type_traits.h"
#  include "xsl/wheel/utils.h"
#  include "xsl/wheel/vec.h"
XSL_NB
using wheel::as_bytes;
using wheel::as_writable_bytes;
using wheel::bool_from_bytes;
using wheel::bool_to_bytes;
using wheel::Defer;
using wheel::dynamic_assert;
using wheel::FixedString;
using wheel::FixedVector;
using wheel::i32_from_bytes;
using wheel::i32_to_bytes;
using wheel::PtrLike;
using wheel::us_map;
namespace type_traits = wheel::type_traits;
XSL_NE
#endif  // XSL_UTILS_WHEEL
