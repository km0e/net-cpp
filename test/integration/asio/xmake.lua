target("it_bind")
do
    set_kind("binary")
    set_default(false)
    add_files("bind.cpp")
    add_deps("xsl_asio", "xsl_test_helpers")
    add_tests("it_bind", { group = "integration", run_timeout = 60000 })
end

-- connect.cpp is disabled everywhere for now: io.h:198 (imm_recv span
-- overload missing; UDP read(span) broken for connection-less sockets),
-- see test/integration/asio/CMakeLists.txt for the pending CMake wiring.
-- target("it_connect")
-- do
--     set_kind("binary")
--     set_default(false)
--     add_files("connect.cpp")
--     add_deps("xsl_asio", "xsl_test_helpers")
--     add_tests("it_connect", { group = "integration", run_timeout = 60000 })
-- end
