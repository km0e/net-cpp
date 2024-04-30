
message(STATUS "[SETUP] tcp server test")

message(STATUS "[DEPENDENCY] tcp server test: xsl_wheel = ${XSL_WHEEL_DIR}")
message(STATUS "[DEPENDENCY] tcp server test: xsl_transport = ${XSL_TRANSPORT_DIR}")

include_directories(
    ${XSL_WHEEL_DIR}
    ${XSL_TRANSPORT_DIR}
)

link_libraries(
    xsl_wheel
    xsl_transport
    ${SPDLOG_LIBRARIES}
    ${CLI11_LIBRARIES}
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
    message(STATUS "[DEPENDENCY] ncat: ${TEST_TOOL_NCAT}, cat: ${TEST_TOOL_CAT}")
endif()

add_executable(tcp_server_test
    ${CMAKE_CURRENT_SOURCE_DIR}/tcp_server_test.cpp
)

add_dependencies(tcp_server_test tcp_client_test)

add_test(NAME tcp_server_test
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/script/test_with_echo_client.py $<TARGET_FILE:tcp_server_test> $<TARGET_FILE:tcp_client_test> ${TEST_IP} ${TEST_PORT}
)


