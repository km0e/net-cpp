/**
 * @file wheel.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Wheel utilities
 * @version 0.11
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once

#ifndef XSL_WHEEL
#  define XSL_WHEEL
#  include "xsl/def.h"
#  include "xsl/wheel/ptr.h"
#  include "xsl/wheel/str.h"
#  include "xsl/wheel/utils.h"
#  include "xsl/wheel/vec.h"
XSL_NB
using wheel::as_bytes;
using wheel::as_writable_bytes;
using wheel::bool_from_bytes;
using wheel::bool_to_bytes;
using wheel::Defer;
using wheel::FixedString;
using wheel::FixedVector;
using wheel::i32_from_bytes;
using wheel::i32_to_bytes;
using wheel::PtrLike;
using wheel::rt_assert;
using wheel::u16_from_bytes;
using wheel::u16_to_bytes;
using wheel::us_map;
XSL_NE
#endif  // XSL_UTILS_WHEEL
