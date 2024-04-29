cmake_minimum_required(VERSION 3.29)
project(httpx)

set(CMAKE_CXX_STANDARD 17)

set(ABSL_PROPAGATE_CXX_STD ON)

include(FetchContent)
FetchContent_Declare(
    abseil
    GIT_REPOSITORY https://github.com/abseil/abseil-cpp.git
    GIT_TAG 20240116.2
)
FetchContent_MakeAvailable(abseil)


add_subdirectory(src)

add_subdirectory(examples)

enable_testing()

add_subdirectory(tests)
