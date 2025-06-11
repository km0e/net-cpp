add_library(xsl_asio STATIC)

file(GLOB_RECURSE XSL_ASIO_SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

target_sources(
    xsl_asio
    PRIVATE
    ${XSL_ASIO_SOURCE_FILES}
)

target_link_libraries(
    xsl_asio
    PRIVATE
    xsl_sys
    xsl_net
    xsl_io
    xsl_coro
)
