add_subdirectory(helper)

add_library(xsl_http STATIC)

set(SOURCE_LIST)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} SOURCE_LIST)

target_sources(xsl_http
    PRIVATE
    ${SOURCE_LIST}
)

target_link_libraries(xsl_http
    PUBLIC
    xsl_transport
    xsl_http_helper
    ${SPDLOG_LIBRARIES}
)
