add_library(xsl_sync STATIC)

set(SOURCE_LIST)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} SOURCE_LIST)

target_sources(xsl_sync
    PRIVATE
    ${SOURCE_LIST}
)

target_link_libraries(xsl_sync
    PUBLIC
    xsl_wheel
)
