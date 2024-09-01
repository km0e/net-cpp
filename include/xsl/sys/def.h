/**
 * @file def.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.11
 * @date 2024-08-31
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_SYS_DEF
#  define XSL_SYS_DEF
#  define XSL_SYS_NB namespace xsl::_sys {
#  define XSL_SYS_NE }
#  include "xsl/io/def.h"

#  include <cstddef>
#  include <string>
XSL_SYS_NB
/// @brief Write file hint
struct WriteFileHint {
  std::string
      path;  ///< file path, must be string, not string_view. Because this function will be called
             ///< in coroutine, and the path may be destroyed before the function is called.
  std::size_t offset;
  std::size_t size;
};

template <class Dev>
concept AsyncRawDeviceLike = requires { typename io::AIOTraits<Dev>::value_type; };

XSL_SYS_NE
#endif
