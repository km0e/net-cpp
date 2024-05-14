
add_subdirectory(wheel)

add_subdirectory(utils)

add_subdirectory(sync)

add_subdirectory(transport)

add_subdirectory(http)

add_library(Xsl STATIC)

target_sources(Xsl
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/xsl.cpp
)

target_link_libraries(Xsl
    PUBLIC
    xsl_utils
    xsl_sync
    xsl_transport
    xsl_http
)
