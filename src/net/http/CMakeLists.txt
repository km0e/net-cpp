add_library(xsl_http STATIC)

file(GLOB_RECURSE XSL_HTTP_SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

target_sources(
    xsl_http
    PRIVATE
    ${XSL_HTTP_SOURCE_FILES}
)

target_link_libraries(xsl_http xsl_tcp xsl_convert)
