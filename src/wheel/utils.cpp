/**
 * @file utils.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-09-01
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "xsl/wheel/def.h"
#include "xsl/wheel/utils.h"

XSL_WHEEL_NB

void rt_assert(bool cond, std::string_view msg, std::source_location loc) {
  rt_assert<bool&, std::string_view>(cond, msg, loc);
}
XSL_WHEEL_NE
