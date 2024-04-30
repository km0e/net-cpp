
include_directories(
    ${XSL_INC_DIR}
)

link_libraries(
    xsl_transport
    ${SPDLOG_LIBRARIES}
    ${CLI11_LIBRARIES}
)

# use nc to test
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

add_executable(tcp_client_test
    ${CMAKE_CURRENT_SOURCE_DIR}/tcp_client_test.cpp
)

add_test(NAME tcp_client_test
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/script/test_with_echo_server.py $<TARGET_FILE:tcp_client_test> ${TEST_HOST} ${TEST_PORT} ${TEST_TOOL_NCAT} ${TEST_TOOL_CAT}
)

add_executable(tcp_client_test_with_poll
    ${CMAKE_CURRENT_SOURCE_DIR}/tcp_client_test_with_poll.cpp
)


add_test(NAME tcp_client_test_with_poll
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/script/test_with_echo_server.py $<TARGET_FILE:tcp_client_test_with_poll> ${TEST_HOST} ${TEST_PORT} ${TEST_TOOL_NCAT} ${TEST_TOOL_CAT}
)

