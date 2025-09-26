add_library(xsl_coro STATIC)

file(GLOB_RECURSE XSL_CORO_SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

target_sources(
    xsl_coro
    PRIVATE
    ${XSL_CORO_SOURCE_FILES}
)

target_link_libraries(xsl_coro PUBLIC xsl_log)
