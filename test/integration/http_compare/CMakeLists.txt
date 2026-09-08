# Correctness comparison between xsl::asio HTTP server and a standalone asio
# HTTP server serving identical routes (performance comparison lives in
# test/benches/http)
find_package(Threads REQUIRED)
xsl_add_test(NAME it_http_compare SOURCES compare.cpp LIBS xsl_asio asio::asio Threads::Threads TIMEOUT 300)
