link_libraries(xsl_tcp)
link_libraries(clilib)
# add_subdirectory(tcp_server)
add_executable(test_bind
    ${CMAKE_CURRENT_SOURCE_DIR}/test_bind.cpp
)

add_executable(test_listen
    ${CMAKE_CURRENT_SOURCE_DIR}/test_listen.cpp
)

add_executable(test_connect
    ${CMAKE_CURRENT_SOURCE_DIR}/test_connect.cpp
)

add_test(NAME test_bind COMMAND test_bind)

add_test(NAME test_listen COMMAND test_listen)

add_test(NAME test_connect COMMAND test_connect)

