/**
 * @file byte.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1.0
 * @date 2024-09-28
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_BYTE
#  define XSL_BYTE
#  include <xsl/def.h>

XSL_NB
using std::byte;

constexpr void bool_to_bytes(bool value, byte* bytes);

constexpr bool bool_from_bytes(const byte* bytes);

XSL_NE
#endif
