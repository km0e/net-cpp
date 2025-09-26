/**
 * @file wheel.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Wheel utilities
 * @version 0.1.2
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once

#ifndef XSL_WHEEL
#  define XSL_WHEEL
#  include <xsl/def.h>
#  include <xsl/wheel/const.h>
#  include <xsl/wheel/rc.h>
#  include <xsl/wheel/static.h>
#  include <xsl/wheel/str.h>
#  include <xsl/wheel/utils.h>
#  include <xsl/wheel/vec.h>
XSL_NB
using wheel::CRLF;
using wheel::Defer;
using wheel::FixedString;
using wheel::FixedVector;
using wheel::Rc;
using wheel::rt_assert;
using wheel::StaticSize;
using wheel::us_map;
XSL_NE
#endif  // XSL_UTILS_WHEEL
