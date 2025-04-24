add_library(xsl_dns STATIC)

file(GLOB_RECURSE SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

target_sources(
    xsl_dns
    PRIVATE
    ${SOURCE_FILES}
)

target_link_libraries(xsl_dns PRIVATE xsl_coro xsl_sys xsl_http)
