find_package(quill QUIET)
if (NOT quill_FOUND)
    message(STATUS "quill not found, adding quill")
    FetchContent_Declare(
        quill
        GIT_REPOSITORY https://github.com/odygrd/quill.git
        GIT_TAG v10.0.1
    )
    FetchContent_MakeAvailable(quill)
endif()

add_library(log STATIC)

target_sources(
    log
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/log.cpp
)

target_link_libraries(log quill)


find_package(CLI11 QUIET)
if (NOT cli11_FOUND)
    message(STATUS "cli11 not found, adding cli11")
    FetchContent_Declare(
        cli11_proj
        QUIET
        GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
        GIT_TAG v2.5.0
    )
    FetchContent_MakeAvailable(cli11_proj)
endif()

add_library(clilib ALIAS CLI11)

find_package(googletest QUIET)
if(NOT googletest_FOUND)
    message(STATUS "googletest not found, adding googletest")
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.17.0
    )
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    # set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

