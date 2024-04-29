cmake_minimum_required(VERSION 3.29)
project(httpx)

set(CMAKE_CXX_STANDARD 17)

set(ABSL_PROPAGATE_CXX_STD ON)

#spdlog

find_package(spdlog REQUIRED)

set(SPDLOG_LIBRARIES spdlog::spdlog_header_only)

add_subdirectory(src)

add_subdirectory(examples)

enable_testing()

add_subdirectory(tests)
