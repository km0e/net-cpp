add_library(xsl_wheel STATIC)

set(SOURCE_LIST)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} SOURCE_LIST)

target_sources(xsl_wheel
    PRIVATE
    ${SOURCE_LIST}
)

target_link_libraries(xsl_wheel
    PUBLIC
    ${SPDLOG_LIBRARIES}
    ${THREAD_POOL_LIBRARIES}
)
