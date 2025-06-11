/**
 * @file dev.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Raw device
 * @version 0.2
 * @date 2024-08-31
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_SYS_DEV
#  define XSL_SYS_DEV
#  include "xsl/sys/def.h"
#  include "xsl/sys/raw.h"

XSL_SYS_NB

/// @brief RawDevice is a wrapper for read-write file descriptor
struct RawDevice : public RawOwner {
  using RawOwner::RawOwner;
};

XSL_SYS_NE
#endif
