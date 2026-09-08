includes("asio")

-- Correctness comparison between xsl::asio HTTP server and a standalone asio
-- HTTP server serving identical routes (port of CMake's it_http_compare;
-- performance comparison lives in test/benches/http).
target("it_http_compare")
do
    set_kind("binary")
    set_default(false)
    add_files("http_compare/compare.cpp")
    add_deps("xsl_asio", "xsl_test_helpers")
    add_packages("asio")
    add_defines("ASIO_STANDALONE", "ASIO_NO_DEPRECATED")
    add_tests("it_http_compare", { group = "integration", run_timeout = 120000 })
end
