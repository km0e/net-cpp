add_executable(tcp_echo
    ${CMAKE_CURRENT_SOURCE_DIR}/tcp_echo.cpp
)

target_link_libraries(tcp_echo clilib xsl_log_ctl)

add_executable(http_server
    ${CMAKE_CURRENT_SOURCE_DIR}/http_server.cpp
)

target_link_libraries(http_server clilib xsl_log_ctl xsl_http)
