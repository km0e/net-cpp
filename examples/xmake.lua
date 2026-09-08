-- CLI11-based demo binaries (asio_example.cpp was removed as dead code)
target("tcp_echo")
do
    set_kind("binary")
    set_default(false)
    add_files("tcp_echo.cpp")
    add_deps("xsl_asio")
    add_packages("cli11")
end

target("udp_echo")
do
    set_kind("binary")
    set_default(false)
    add_files("udp_echo.cpp")
    add_deps("xsl_asio")
    add_packages("cli11")
end

target("udp_client")
do
    set_kind("binary")
    set_default(false)
    add_files("udp_client.cpp")
    add_deps("xsl_asio")
    add_packages("cli11")
end

target("http_client")
do
    set_kind("binary")
    set_default(false)
    add_files("http_client.cpp")
    add_deps("xsl_asio")
    add_packages("cli11")
end

target("http_server")
do
    set_kind("binary")
    set_default(false)
    add_files("http_server.cpp")
    add_deps("xsl_asio")
    add_packages("cli11")
end
