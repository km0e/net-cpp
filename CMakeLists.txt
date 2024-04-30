cmake_minimum_required(VERSION 3.29)
project(net-pp)

set(CMAKE_CXX_STANDARD 17)

set(ABSL_PROPAGATE_CXX_STD ON)

#spdlog

find_package(spdlog REQUIRED)

set(SPDLOG_LIBRARIES spdlog::spdlog_header_only)

include(FetchContent)
FetchContent_Declare(
    cli11_proj
    QUIET
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    GIT_TAG v2.3.2
)

FetchContent_MakeAvailable(cli11_proj)

set(CLI11_LIBRARIES CLI11::CLI11)

add_subdirectory(include)

add_subdirectory(src)

add_subdirectory(examples)

enable_testing()

add_subdirectory(tests)
