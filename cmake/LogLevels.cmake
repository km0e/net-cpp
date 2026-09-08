# Log level helpers shared by the root CMakeLists.
#
# quill's LogMacros.h defines the compile-time switches:
#   TRACE_L3=0 TRACE_L2=1 TRACE_L1=2 DEBUG=3 INFO=4 NOTICE=5
#   WARNING=6 ERROR=7 CRITICAL=8   (no NONE constant; default -1 = all enabled)
# There is no NONE constant, so OFF maps to 9 (= CRITICAL + 1), which compiles
# every log statement out.

# xsl_log_level_value(<LEVEL> <OUT_VAR>)
#   LEVEL: OFF|TRACE|DEBUG|INFO|WARNING|ERROR|CRITICAL
#   OUT_VAR receives the value for QUILL_COMPILE_ACTIVE_LOG_LEVEL.
function(xsl_log_level_value LEVEL OUT_VAR)
  set(_levels OFF TRACE DEBUG INFO WARNING ERROR CRITICAL)
  if(NOT LEVEL IN_LIST _levels)
    message(FATAL_ERROR "Unknown log level '${LEVEL}'. Valid values: ${_levels}")
  endif()
  if(LEVEL STREQUAL "OFF")
    set(${OUT_VAR} 9 PARENT_SCOPE)
  elseif(LEVEL STREQUAL "TRACE")
    # log_trace() emits LOG_TRACE_L1, so filtering at L1 is exact
    set(${OUT_VAR} QUILL_COMPILE_ACTIVE_LOG_LEVEL_TRACE_L1 PARENT_SCOPE)
  else()
    set(${OUT_VAR} QUILL_COMPILE_ACTIVE_LOG_LEVEL_${LEVEL} PARENT_SCOPE)
  endif()
endfunction()

# xsl_log_level_n(<LEVEL> <OUT_VAR>)
#   0-based severity index for XSL_CORO_LOG_LEVEL_N
#   (TRACE=0 ... CRITICAL=5, OFF=6).
function(xsl_log_level_n LEVEL OUT_VAR)
  set(_levels TRACE DEBUG INFO WARNING ERROR CRITICAL OFF)
  list(FIND _levels "${LEVEL}" _idx)
  if(_idx EQUAL -1)
    message(FATAL_ERROR "Unknown log level '${LEVEL}'. Valid values: ${_levels}")
  endif()
  set(${OUT_VAR} ${_idx} PARENT_SCOPE)
endfunction()
