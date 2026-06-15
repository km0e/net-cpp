# Self-contained integration test (creates its own TCP server)
add_executable(it_bind bind.cpp)
target_link_libraries(it_bind PRIVATE xsl_asio clilib GTest::gtest_main)
add_test(NAME it_bind COMMAND it_bind)

# connect.cpp needs Python echo server + library fix:
# io.h:198 — imm_recv span overload missing; UDP read(span) path is
# broken for connection-less sockets.
# Once io.h is fixed, uncomment below:
#
# find_package(Python3 QUIET)
# if(Python3_FOUND)
#   add_executable(it_connect connect.cpp)
#   target_link_libraries(it_connect PRIVATE xsl_asio clilib GTest::gtest_main)
#   add_test(NAME it_connect
#     COMMAND ${Python3_EXECUTABLE}
#             ${CMAKE_SOURCE_DIR}/test/script/with_echo_server.py
#             $<TARGET_FILE:it_connect>
#   )
#   set_tests_properties(it_connect PROPERTIES
#     ENVIRONMENT "PYTHONPATH=${CMAKE_SOURCE_DIR}/test/script"
#   )
# endif()
