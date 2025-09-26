add_library(xsl_net STATIC)

file(GLOB_RECURSE XSL_NET_SOURCE_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

target_sources(
    xsl_net
    PRIVATE
    ${XSL_NET_SOURCE_FILES}
)

target_link_libraries(xsl_net PUBLIC xsl_log xsl_sys xsl_wheel)
