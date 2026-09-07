add_executable(net net.cpp)
target_link_libraries(net PRIVATE xsl_asio GTest::gtest_main)
add_test(NAME net COMMAND net)

add_executable(ctx_stop ctx_stop.cpp)
target_link_libraries(ctx_stop PRIVATE xsl_asio GTest::gtest_main)
add_test(NAME ctx_stop COMMAND ctx_stop)
