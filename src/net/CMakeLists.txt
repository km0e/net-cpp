add_subdirectory(transport)
add_subdirectory(http)
add_subdirectory(sync)

add_library(xsl_tcp STATIC)

target_sources(
    xsl_tcp
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/tcp.cpp
)

target_link_libraries(xsl_tcp PRIVATE xsl_sys)
