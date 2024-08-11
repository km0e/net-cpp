cmake_minimum_required(VERSION 3.14...3.22)

# ---- Project ----

# Note: update this to your new project's name and version
project(
  Xsl
  VERSION 1.0
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS True)

set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/build)

# ---- Include guards ----
if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(
    FATAL_ERROR
    "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there."
  )
endif()

Include(FetchContent)

find_package(quill QUIET)
if (NOT quill_FOUND)
  message(STATUS "quill not found, Fetching quill")
  FetchContent_Declare(
    quill
    GIT_REPOSITORY https://github.com/odygrd/quill.git
    GIT_TAG        v4.5.0
  )
  FetchContent_MakeAvailable(quill)
endif()

add_library(loglib ALIAS quill)

find_package(cli11 QUIET)
if (NOT cli11_FOUND)
  message(STATUS "cli11 not found, Fetching cli11")
  FetchContent_Declare(
    cli11_proj
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    GIT_TAG v2.4.2
  )
  FetchContent_MakeAvailable(cli11_proj)
endif()

add_library(clilib ALIAS CLI11)

find_package(googletest QUIET)
if(NOT googletest_FOUND)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
  )
  # For Windows: Prevent overriding the parent project's compiler/linker settings
  # set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
endif()

# add_link_options(-fuse-ld=mold)

# --- Logging ----
# set(LOG_LEVEL "INFO" CACHE STRING "Set the log level (NONE, TRACE, DEBUG, INFO, WARNING, ERROR, CRITICAL)")
set(LOG_LEVEL "OFF")
set(LOG_LEVEL CACHE STRING "Set the log level (OFF, TRACE, DEBUG, INFO, WARNING, ERROR, CRITICAL)")

set_property(CACHE LOG_LEVEL PROPERTY STRINGS OFF  TRACE  DEBUG  INFO  WARNING  ERROR  CRITICAL)
# 打印选项值
message(STATUS "LOG_LEVEL: ${LOG_LEVEL}")

# 定义一个函数来为目标设置日志级别
function(set_log_level target)
    if(LOG_LEVEL STREQUAL "OFF")
        target_compile_definitions(${target} PUBLIC QUILL_COMPILE_ACTIVE_LOG_LEVEL=8)
    elseif(LOG_LEVEL STREQUAL "TRACE")
        target_compile_definitions(${target} PUBLIC QUILL_COMPILE_ACTIVE_LOG_LEVEL=QUILL_COMPILE_ACTIVE_LOG_LEVEL_TRACE_L3)
    elseif(LOG_LEVEL STREQUAL "DEBUG")
        target_compile_definitions(${target} PUBLIC QUILL_COMPILE_ACTIVE_LOG_LEVEL=QUILL_COMPILE_ACTIVE_LOG_LEVEL_DEBUG)
    elseif(LOG_LEVEL STREQUAL "INFO")
        target_compile_definitions(${target} PUBLIC QUILL_COMPILE_ACTIVE_LOG_LEVEL=QUILL_COMPILE_ACTIVE_LOG_LEVEL_INFO)
    elseif(LOG_LEVEL STREQUAL "WARNING")
        target_compile_definitions(${target} PUBLIC QUILL_COMPILE_ACTIVE_LOG_LEVEL=QUILL_COMPILE_ACTIVE_LOG_LEVEL_WARNING)
    elseif(LOG_LEVEL STREQUAL "ERROR")
        target_compile_definitions(${target} PUBLIC QUILL_COMPILE_ACTIVE_LOG_LEVEL=QUILL_COMPILE_ACTIVE_LOG_LEVEL_ERROR)
    elseif(LOG_LEVEL STREQUAL "CRITICAL")
        target_compile_definitions(${target} PUBLIC QUILL_COMPILE_ACTIVE_LOG_LEVEL=QUILL_COMPILE_ACTIVE_LOG_LEVEL_CRITICAL)
    else()
        message(STATUS "Unknown log level: ${LOG_LEVEL}")
    endif()
endfunction()

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

include_directories(${PROJECT_SOURCE_DIR}/include)

add_subdirectory(src)

add_subdirectory(examples)

# ---- Add XslTests ----
enable_testing()

add_subdirectory(test)
