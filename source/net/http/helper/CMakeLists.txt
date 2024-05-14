add_library(xsl_http_helper STATIC)

set(SOURCE_LIST)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} SOURCE_LIST)

target_sources(xsl_http_helper
    PRIVATE
    ${SOURCE_LIST}
)

target_link_libraries(xsl_http_helper
    PUBLIC
    ${SPDLOG_LIBRARIES}
)
