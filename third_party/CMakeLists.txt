find_package(quill QUIET)
if (NOT quill_FOUND)
    message(STATUS "quill not found, adding quill")
    add_subdirectory(quill)
endif()

add_library(log STATIC)

target_sources(
    log
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/log.cpp
)

target_link_libraries(log quill)
