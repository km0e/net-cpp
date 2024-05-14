add_library(xsl_tcp STATIC)

set(SOURCE_LIST)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} SOURCE_LIST)

target_sources(xsl_tcp
    PRIVATE
    ${SOURCE_LIST}
)

target_link_libraries(xsl_tcp
    PUBLIC
    xsl_utils
    xsl_sync
)
