/**
 * @file channel.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Signal for coroutines
 * @version 0.1.0
 * @date 2025-08-05
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once

#ifndef XSL_CORO_CHANNEL
#  define XSL_CORO_CHANNEL
#  include <xsl/coro/channel/def.h>
#  include <xsl/coro/channel/spsc.h>
#  include <xsl/coro/def.h>

#  include <cassert>
#  include <cstddef>
XSL_CORO_NB
template <class T, std::size_t MaxElements = 1024>
using SPSCChannel = AnyChannel<SPSCChannelStorage<T>, MaxElements>;
XSL_CORO_NE
#endif
