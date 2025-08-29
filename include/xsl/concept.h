/**
 * @file concept.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Concepts for wheel
 * @version 0.1.0
 * @date 2025-09-06
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once
#ifndef XSL_WHEEL_CONCEPT
#  define XSL_WHEEL_CONCEPT
#  include <xsl/def.h>

XSL_NB
template <class T, class... Ts>
concept exists = (std::same_as<T, Ts> || ...);
XSL_NE

#endif
