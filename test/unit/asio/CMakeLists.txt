add_executable(net net.cpp)
target_link_libraries(net PRIVATE xsl_asio GTest::gtest_main)
add_test(NAME net COMMAND net)
