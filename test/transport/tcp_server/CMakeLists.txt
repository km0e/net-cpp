
link_libraries(
    xsl_transport
)

# use nc to test
find_program(TEST_TOOL_NCAT ncat)
find_program(TEST_TOOL_CAT cat)
find_package(Python3 COMPONENTS Interpreter)

set(TEST_IP "127.0.0.1")
set(TEST_PORT 12345)

if(NOT TEST_TOOL_NCAT OR NOT TEST_TOOL_CAT)
    message(FATAL_ERROR "ncat or cat not found")
else()
    message(STATUS "[tcp_server_test] ncat found: ${TEST_TOOL_NCAT}")
endif()

add_executable(test_tcp_server
    ${CMAKE_CURRENT_SOURCE_DIR}/test_tcp_server.cpp
)

add_dependencies(test_tcp_server test_tcp_client)

add_test(NAME tcp_server_test
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/script/test_with_echo_client.py ${TEST_IP} ${TEST_PORT} $<TARGET_FILE:test_tcp_client> $<TARGET_FILE:test_tcp_server>
)

