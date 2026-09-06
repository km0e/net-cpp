# Correctness comparison between xsl::asio HTTP server and a standalone asio
# HTTP server serving identical routes (performance comparison lives in
# test/benches/http)
add_executable(it_http_compare compare.cpp)
find_package(Threads REQUIRED)
target_link_libraries(it_http_compare PRIVATE xsl_asio asio::asio GTest::gtest_main Threads::Threads)
add_test(NAME it_http_compare COMMAND it_http_compare)
