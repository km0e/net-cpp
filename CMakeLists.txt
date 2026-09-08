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

# This project does not use C++20 modules; scanning every TU for module
# dependencies (CMake's default for C++20+) also makes ~95%% of compilations
# uncacheable for ccache and adds a preprocessing pass per file.
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

# ---- Options ----

set(XSL_LOG_LEVEL "INFO" CACHE STRING "Set the log level (OFF, TRACE, DEBUG, INFO, WARNING, ERROR, CRITICAL)")
set_property(CACHE XSL_LOG_LEVEL PROPERTY STRINGS OFF TRACE DEBUG INFO WARNING ERROR CRITICAL)

set(XSL_CORO_LOG_LEVEL "OFF" CACHE STRING "Coro log level (OFF, TRACE, DEBUG, INFO, WARNING, ERROR, CRITICAL)")
set_property(CACHE XSL_CORO_LOG_LEVEL PROPERTY STRINGS OFF TRACE DEBUG INFO WARNING ERROR CRITICAL)

option(XSL_BUILD_EXAMPLES "Build examples" ON)
option(XSL_BUILD_TESTS "Build tests" ON)
option(XSL_UNITY_BUILD "Unity (jumbo) build: faster full rebuilds, worse incrementals, may hit duplicate static names" OFF)
option(XSL_PCH "Precompile xsl/log.h + quill headers for every project target" OFF)
set(XSL_SANITIZE "" CACHE STRING "Enable sanitizers, e.g. 'address;undefined' (empty = off)")

# ---- Include guards ----
if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(
    FATAL_ERROR
    "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there."
  )
endif()

# ---- Build type ----
get_property(_xsl_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(NOT _xsl_multi_config AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)
  message(STATUS "CMAKE_BUILD_TYPE not set, defaulting to Debug")
endif()

# ---- Toolchain: mold linker, ccache ----
find_program(MOLD_EXECUTABLE mold)
if(MOLD_EXECUTABLE)
  add_link_options(-fuse-ld=mold)
endif()

find_program(CCACHE_EXECUTABLE ccache)
if(CCACHE_EXECUTABLE)
  set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_EXECUTABLE}")
  message(STATUS "Using ccache: ${CCACHE_EXECUTABLE}")
endif()

# ---- Sanitizers (opt-in, see CMakePresets.json 'asan') ----
if(XSL_SANITIZE)
  # XSL_SANITIZE is a CMake list (e.g. "address;undefined"); emit one flag per item
  foreach(_sanitizer IN LISTS XSL_SANITIZE)
    add_compile_options(-fsanitize=${_sanitizer} -fno-omit-frame-pointer)
    add_link_options(-fsanitize=${_sanitizer})
  endforeach()
endif()

# ---- Unity build (opt-in) ----
if(XSL_UNITY_BUILD)
  set(CMAKE_UNITY_BUILD ON)
  set(CMAKE_UNITY_BUILD_BATCH_SIZE 8)
endif()

# ---- Logging ----
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(LogLevels)
xsl_log_level_value("${XSL_LOG_LEVEL}" XSL_QUILL_LOG_LEVEL)
message(STATUS "XSL_LOG_LEVEL: ${XSL_LOG_LEVEL} (QUILL_COMPILE_ACTIVE_LOG_LEVEL=${XSL_QUILL_LOG_LEVEL})")
message(STATUS "XSL_CORO_LOG_LEVEL: ${XSL_CORO_LOG_LEVEL}")

# ---- Dependencies (CPM: quill, asio, CLI11, googletest) ----
include(cmake/deps.cmake)

# ---- Project-wide warnings ----
# Deliberately placed *after* deps.cmake: CPM already created the third-party
# targets above, so these flags only apply to xsl targets, in every config.
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
elseif(MSVC)
  add_compile_options(/W4)
  add_compile_definitions(_SILENCE_CXX17_RESULT_OF_DEPRECATION_WARNING)
endif()

find_package(OpenSSL REQUIRED COMPONENTS SSL Crypto)

include(TestHelpers)

add_library(xsl_log STATIC)

target_sources(
   xsl_log
    PRIVATE
    ${PROJECT_SOURCE_DIR}/src/log.cpp
)

target_link_libraries(xsl_log loglib)
target_include_directories(xsl_log PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_definitions(xsl_log PUBLIC QUILL_COMPILE_ACTIVE_LOG_LEVEL=${XSL_QUILL_LOG_LEVEL})

if(NOT XSL_CORO_LOG_LEVEL STREQUAL "OFF")
  xsl_log_level_n("${XSL_CORO_LOG_LEVEL}" XSL_CORO_LOG_LEVEL_N)
  target_compile_definitions(xsl_log PUBLIC
    XSL_CORO_LOG_ENABLED=1
    XSL_CORO_LOG_LEVEL_N=${XSL_CORO_LOG_LEVEL_N}
  )
endif()

if(XSL_PCH)
  # PUBLIC: every target linking xsl_log (i.e. all of them) reuses the PCH
  target_precompile_headers(xsl_log PUBLIC
    <quill/Logger.h>
    <quill/LogMacros.h>
    <xsl/log.h>
  )
endif()

add_subdirectory(src)

if(XSL_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

# ---- Add XslTests ----
if(XSL_BUILD_TESTS)
  enable_testing()
  add_subdirectory(test)
endif()

# ---- Install ----
# quill is installed too (QUILL_ENABLE_INSTALL=ON): the exported targets
# reference quill::quill, resolved by find_dependency in XslConfig.cmake.
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(
  TARGETS xsl xsl_asio xsl_net xsl_coro xsl_sys xsl_wheel xsl_log
  EXPORT XslTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(DIRECTORY "${PROJECT_SOURCE_DIR}/include/xsl" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

configure_package_config_file(
  "${PROJECT_SOURCE_DIR}/cmake/XslConfig.cmake.in" "${PROJECT_BINARY_DIR}/cmake/XslConfig.cmake"
  INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/xsl"
)
write_basic_package_version_file(
  "${PROJECT_BINARY_DIR}/cmake/XslConfigVersion.cmake"
  COMPATIBILITY SameMajorVersion
)
install(
  EXPORT XslTargets
  NAMESPACE xsl::
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/xsl"
)
install(
  FILES
    "${PROJECT_BINARY_DIR}/cmake/XslConfig.cmake"
    "${PROJECT_BINARY_DIR}/cmake/XslConfigVersion.cmake"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/xsl"
)
# lowercase alias so find_package(xsl) finds it too (project name is Xsl)
install(
  FILES "${PROJECT_BINARY_DIR}/cmake/XslConfig.cmake"
  RENAME xslConfig.cmake
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/xsl"
)
