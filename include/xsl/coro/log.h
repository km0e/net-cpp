/**
 * @file log.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Coroutine logging control
 * @version 0.1.0
 * @date 2025-09-11
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once
#ifndef XSL_CORO_LOG
#  define XSL_CORO_LOG
#  include <xsl/coro/def.h>
#  include <xsl/log.h>

XSL_CORO_NB
extern xsl::LogCtl2 logger_coro;
#  define co_trace(fmt, ...) LOG_TRACE_L1(xsl::coro::logger_coro.logger, fmt, ##__VA_ARGS__)

#  define co_debug(fmt, ...) LOG_DEBUG(xsl::coro::logger_coro.logger, fmt, ##__VA_ARGS__)

#  define co_info(fmt, ...) LOG_INFO(xsl::coro::logger_coro.logger, fmt, ##__VA_ARGS__)

#  define co_warning(fmt, ...) LOG_WARNING(xsl::coro::logger_coro.logger, fmt, ##__VA_ARGS__)

#  define co_error(fmt, ...) LOG_ERROR(xsl::coro::logger_coro.logger, fmt, ##__VA_ARGS__)

#  define co_critical(fmt, ...) LOG_CRITICAL(xsl::coro::logger_coro.logger, fmt, ##__VA_ARGS__)

XSL_CORO_NE
#endif
