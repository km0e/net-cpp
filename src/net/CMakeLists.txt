add_subdirectory(http)

add_library(xsl_net STATIC)

target_sources(
    xsl_net
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/net.cpp
)

target_link_libraries(xsl_net PRIVATE xsl_coro xsl_sys xsl_http)
