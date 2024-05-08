
link_libraries(
    xsl_http
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

add_executable(http_server_test
    ${CMAKE_CURRENT_SOURCE_DIR}/http_server_test.cpp
)

# add_dependencies(http_server_test tcp_client_test)

# add_test(NAME http_server_test
#     COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/script/test_with_echo_client.py $<TARGET_FILE:http_server_test> $<TARGET_FILE:tcp_client_test> ${TEST_IP} ${TEST_PORT}
# )


