cmake_minimum_required(VERSION 4.0)

# ---- Project ----

# Note: update this to your new project's name and version
project(
  Xsl
  VERSION 1.0
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS True)

# ---- Include guards ----
if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(
    FATAL_ERROR
    "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there."
  )
endif()

add_link_options(-fuse-ld=mold)

# --- Logging ----
set(XSL_LOG_LEVEL "INFO" CACHE STRING "Set the log level (NONE, TRACE, DEBUG, INFO, WARNING, ERROR, CRITICAL)")

set_property(CACHE XSL_LOG_LEVEL PROPERTY STRINGS OFF TRACE DEBUG INFO WARNING ERROR CRITICAL)
message(STATUS "XSL_LOG_LEVEL: ${XSL_LOG_LEVEL}")

set(XSL_CORO_LOG_LEVEL "OFF" CACHE STRING "Coro log level (OFF, TRACE, DEBUG, INFO, WARNING, ERROR, CRITICAL)")
set_property(CACHE XSL_CORO_LOG_LEVEL PROPERTY STRINGS OFF TRACE DEBUG INFO WARNING ERROR CRITICAL)
message(STATUS "XSL_CORO_LOG_LEVEL: ${XSL_CORO_LOG_LEVEL}")

if(XSL_CORO_LOG_LEVEL STREQUAL "TRACE")
  set(XSL_CORO_LOG_LEVEL_N 0)
elseif(XSL_CORO_LOG_LEVEL STREQUAL "DEBUG")
  set(XSL_CORO_LOG_LEVEL_N 1)
elseif(XSL_CORO_LOG_LEVEL STREQUAL "INFO")
  set(XSL_CORO_LOG_LEVEL_N 2)
elseif(XSL_CORO_LOG_LEVEL STREQUAL "WARNING")
  set(XSL_CORO_LOG_LEVEL_N 3)
elseif(XSL_CORO_LOG_LEVEL STREQUAL "ERROR")
  set(XSL_CORO_LOG_LEVEL_N 4)
elseif(XSL_CORO_LOG_LEVEL STREQUAL "CRITICAL")
  set(XSL_CORO_LOG_LEVEL_N 5)
else()
  set(XSL_CORO_LOG_LEVEL_N 6)  # OFF
endif()

# --- Import tools ----
# enable compiler warnings if is debug build
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    add_compile_options(-Wall -Wpedantic -Wextra)
    # add_compile_options(-Wall -Wpedantic -Wextra -Werror)
  elseif(MSVC)
    add_compile_options(/W4 /WX)
    add_compile_definitions(_SILENCE_CXX17_RESULT_OF_DEPRECATION_WARNING)
  endif()
endif()

include(cmake/deps.cmake)

find_package(OpenSSL REQUIRED COMPONENTS SSL Crypto)

add_library(xsl_log STATIC)

target_sources(
   xsl_log
    PRIVATE
    ${PROJECT_SOURCE_DIR}/src/log.cpp
)

target_link_libraries(xsl_log loglib)
target_include_directories(xsl_log PUBLIC ${PROJECT_SOURCE_DIR}/include)

if(XSL_LOG_LEVEL STREQUAL "OFF")
  target_compile_definitions(xsl_log PUBLIC QUILL_COMPILE_ACTIVE_XSL_LOG_LEVEL=8)
elseif(XSL_LOG_LEVEL STREQUAL "TRACE")
  target_compile_definitions(xsl_log PUBLIC QUILL_COMPILE_ACTIVE_XSL_LOG_LEVEL=QUILL_COMPILE_ACTIVE_XSL_LOG_LEVEL_TRACE_L3)
elseif(XSL_LOG_LEVEL STREQUAL "DEBUG")
  target_compile_definitions(xsl_log PUBLIC QUILL_COMPILE_ACTIVE_XSL_LOG_LEVEL=QUILL_COMPILE_ACTIVE_XSL_LOG_LEVEL_DEBUG)
elseif(XSL_LOG_LEVEL STREQUAL "INFO")
  target_compile_definitions(xsl_log PUBLIC QUILL_COMPILE_ACTIVE_XSL_LOG_LEVEL=QUILL_COMPILE_ACTIVE_XSL_LOG_LEVEL_INFO)
elseif(XSL_LOG_LEVEL STREQUAL "WARNING")
  target_compile_definitions(xsl_log PUBLIC QUILL_COMPILE_ACTIVE_XSL_LOG_LEVEL=QUILL_COMPILE_ACTIVE_XSL_LOG_LEVEL_WARNING)
elseif(XSL_LOG_LEVEL STREQUAL "ERROR")
  target_compile_definitions(xsl_log PUBLIC QUILL_COMPILE_ACTIVE_XSL_LOG_LEVEL=QUILL_COMPILE_ACTIVE_XSL_LOG_LEVEL_ERROR)
elseif(XSL_LOG_LEVEL STREQUAL "CRITICAL")
  target_compile_definitions(xsl_log PUBLIC QUILL_COMPILE_ACTIVE_XSL_LOG_LEVEL=QUILL_COMPILE_ACTIVE_XSL_LOG_LEVEL_CRITICAL)
else()
  message(FATAL_ERROR "Unknown XSL_LOG_LEVEL '${XSL_LOG_LEVEL}'. Valid values: OFF, TRACE, DEBUG, INFO, WARNING, ERROR, CRITICAL")
endif()

if(NOT XSL_CORO_LOG_LEVEL STREQUAL "OFF")
  target_compile_definitions(xsl_log PUBLIC
    XSL_CORO_LOG_ENABLED=1
    XSL_CORO_LOG_LEVEL_N=${XSL_CORO_LOG_LEVEL_N}
  )
endif()

add_subdirectory(src)

add_subdirectory(examples)

# ---- Add XslTests ----
enable_testing()

add_subdirectory(test)
