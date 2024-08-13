add_library(xsl_log_ctl STATIC)

target_sources(
    xsl_log_ctl
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/logctl.cpp
)

target_link_libraries(xsl_log_ctl loglib)

set_log_level(xsl_log_ctl)

link_libraries(xsl_log_ctl)

add_subdirectory(sys)
add_subdirectory(net)
add_subdirectory(coro)
add_subdirectory(wheel)

add_library(xsl_convert STATIC)

target_sources(
    xsl_convert
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/convert.cpp
)

add_library(xsl STATIC)

target_sources(
    xsl
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/xsl.cpp
)

target_link_libraries(xsl xsl_net xsl_coro)
