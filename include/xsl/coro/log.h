/**
 * @file log.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Coroutine logging — real-time fprintf to stderr, level-filtered
 * @version 0.3.0
 * @date 2025-09-11
 *
 * @copyright Copyright (c) 2025
 *
 * Controlled by XSL_CORO_LOG_LEVEL (cmake option, defaults to OFF).
 * Levels (integer): TRACE=0 DEBUG=1 INFO=2 WARNING=3 ERROR=4 CRITICAL=5
 * Only messages at or above the configured level are emitted.
 */
#pragma once
#ifndef XSL_CORO_LOG
#  define XSL_CORO_LOG

#  if defined(XSL_CORO_LOG_ENABLED) && XSL_CORO_LOG_ENABLED
#    include <cstdio>
#    include <thread>
#    include <time.h>

#    define XSL_CORO_LOG_LVL_TRACE     0
#    define XSL_CORO_LOG_LVL_DEBUG     1
#    define XSL_CORO_LOG_LVL_INFO      2
#    define XSL_CORO_LOG_LVL_WARNING   3
#    define XSL_CORO_LOG_LVL_ERROR     4
#    define XSL_CORO_LOG_LVL_CRITICAL  5

#    if !defined(XSL_CORO_LOG_LEVEL_N)
#      define XSL_CORO_LOG_LEVEL_N 0  // default: TRACE if enabled without level
#    endif

#    define XSL_CORO_LOG_IMPL(level, lvl, fmt, ...)                                           \
      do {                                                                                     \
        if constexpr ((lvl) >= XSL_CORO_LOG_LEVEL_N) {                                        \
          auto _tid = std::hash<std::thread::id>{}(std::this_thread::get_id()) % 10000;        \
          auto _now = time(nullptr);                                                           \
          struct tm _tm;                                                                       \
          localtime_r(&_now, &_tm);                                                            \
          fprintf(stderr, "[%02d:%02d:%02d] [%s] [%04d] [%s:%d] " fmt "\n",                   \
                  _tm.tm_hour, _tm.tm_min, _tm.tm_sec, level, (int)_tid, __FILE__, __LINE__,  \
                  ##__VA_ARGS__);                                                              \
          fflush(stderr);                                                                      \
        }                                                                                      \
      } while (0)

#    define co_trace(fmt, ...)    XSL_CORO_LOG_IMPL("T", XSL_CORO_LOG_LVL_TRACE,    fmt, ##__VA_ARGS__)
#    define co_debug(fmt, ...)    XSL_CORO_LOG_IMPL("D", XSL_CORO_LOG_LVL_DEBUG,    fmt, ##__VA_ARGS__)
#    define co_info(fmt, ...)     XSL_CORO_LOG_IMPL("I", XSL_CORO_LOG_LVL_INFO,     fmt, ##__VA_ARGS__)
#    define co_warning(fmt, ...)  XSL_CORO_LOG_IMPL("W", XSL_CORO_LOG_LVL_WARNING,  fmt, ##__VA_ARGS__)
#    define co_error(fmt, ...)    XSL_CORO_LOG_IMPL("E", XSL_CORO_LOG_LVL_ERROR,    fmt, ##__VA_ARGS__)
#    define co_critical(fmt, ...) XSL_CORO_LOG_IMPL("C", XSL_CORO_LOG_LVL_CRITICAL, fmt, ##__VA_ARGS__)

#  else

#    define co_trace(fmt, ...)    ((void)0)
#    define co_debug(fmt, ...)    ((void)0)
#    define co_info(fmt, ...)     ((void)0)
#    define co_warning(fmt, ...)  ((void)0)
#    define co_error(fmt, ...)    ((void)0)
#    define co_critical(fmt, ...) ((void)0)

#  endif

#  include <xsl/coro/def.h>

XSL_CORO_NB
inline void flush_log() {
#if defined(XSL_CORO_LOG_ENABLED) && XSL_CORO_LOG_ENABLED
  fflush(stderr);
#endif
}
XSL_CORO_NE
#endif
