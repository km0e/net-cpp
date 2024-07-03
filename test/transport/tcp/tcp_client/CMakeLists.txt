# use nc to test
link_libraries(clilib)
find_program(TEST_TOOL_NCAT ncat)
find_program(TEST_TOOL_CAT cat)
find_package(Python3 COMPONENTS Interpreter)

set(TEST_HOST "127.0.0.1")
set(TEST_PORT 12345)

if(NOT TEST_TOOL_NCAT OR NOT TEST_TOOL_CAT)
    message(FATAL_ERROR "ncat or cat not found")
else()
    message(STATUS "[tcp_client_test] ncat: ${TEST_TOOL_NCAT}")
endif()

add_executable(test_tcp_client
    ${CMAKE_CURRENT_SOURCE_DIR}/test_tcp_client.cpp
)

add_executable(test_tcp_client_with_poll
    ${CMAKE_CURRENT_SOURCE_DIR}/test_tcp_client_with_poll.cpp
)

add_test(NAME tcp_client_test
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/script/test_with_echo_server.py ${TEST_HOST} ${TEST_PORT} ${TEST_TOOL_NCAT} ${TEST_TOOL_CAT} $<TARGET_FILE:test_tcp_client> $<TARGET_FILE:test_tcp_client_with_poll>
)
