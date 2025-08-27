/**
 * @file coro.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Coroutine utilities
 * @version 0.3.0
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_CORO
#  define XSL_CORO
#  include <xsl/coro/channel.h>
#  include <xsl/coro/core/block.h>
#  include <xsl/coro/core/detach.h>
#  include <xsl/coro/core/executor.h>
#  include <xsl/coro/core/task.h>
#  include <xsl/coro/error.h>
#  include <xsl/coro/guard.h>
#  include <xsl/coro/pub_sub.h>
#  include <xsl/coro/signal.h>
#  include <xsl/def.h>
XSL_NB
using namespace coro;
XSL_NE
#endif
