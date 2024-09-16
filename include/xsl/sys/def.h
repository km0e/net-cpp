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

#  include <fcntl.h>
XSL_SYS_NB
using RawHandle = int;

enum class DeviceAttribute {
  NonBlocking = O_NONBLOCK,
  CloseOnExec = O_CLOEXEC,
};

template <class Dev>
concept AsyncRawDeviceLike = requires { typename io::AIOTraits<Dev>::value_type; };

XSL_SYS_NE
#endif
