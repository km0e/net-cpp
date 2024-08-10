add_executable(echo_server
    ${CMAKE_CURRENT_SOURCE_DIR}/echo_server.cpp
)

target_link_libraries(echo_server clilib xsl_log_ctl xsl_tcp)

add_executable(http_server
    ${CMAKE_CURRENT_SOURCE_DIR}/http_server.cpp
)

target_link_libraries(http_server clilib xsl_log_ctl xsl_http)
