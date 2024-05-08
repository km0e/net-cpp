cmake_minimum_required(VERSION 3.14...3.22)

# ---- Project ----

# Note: update this to your new project's name and version
project(
  Xsl
  VERSION 1.0
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)

# ---- Include guards ----
if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(
    FATAL_ERROR
    "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there."
  )
endif()

find_package(spdlog REQUIRED)

find_package(CLI11 REQUIRED)

find_package(fmt REQUIRED)

set(CLI11_LIBRARIES CLI11::CLI11)

set(FMT_LIBRARIES fmt::fmt)

add_link_options(-fuse-ld=mold)

add_compile_options(-O3)

# --- Import tools ----
# enable compiler warnings if is debug build
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    add_compile_options(-Wall -Wpedantic -Wextra -Werror)
  elseif(MSVC)
    add_compile_options(/W4 /WX)
    add_compile_definitions(_SILENCE_CXX17_RESULT_OF_DEPRECATION_WARNING)
  endif()
endif()

add_subdirectory(include)

include_directories(${XSL_INC_DIR})

add_subdirectory(source)

add_subdirectory(examples)

# ---- Add XslTests ----
enable_testing()

add_subdirectory(test)
