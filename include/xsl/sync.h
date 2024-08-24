/**
 * @file sync.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Synchronization primitives.
 * @version 0.1
 * @date 2024-08-23
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_SYNC
#  define XSL_SYNC
#  include "xsl/def.h"
#  include "xsl/sync/mutex.h"
#  include "xsl/sync/spsc.h"
XSL_NB
namespace sync {}  // namespace sync
using _sync::LockGuard;
using _sync::ShardGuard;
using _sync::ShardRes;
using _sync::spsc;
XSL_NE
#endif
