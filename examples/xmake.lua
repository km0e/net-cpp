add_deps("prepare", "cli")
target("tcp_echo")
do
    set_kind("binary")
    add_files("tcp_echo.cpp")
    add_deps("xsl")
end

target("http_client")
do
    set_kind("binary")
    add_files("http_client.cpp")
    add_deps("xsl_asio")
end

target("http_server")
do
    set_kind("binary")
    add_files("http_server.cpp")
    add_deps("xsl_asio")
end

target("udp_client")
do
    set_kind("binary")
    add_files("udp_client.cpp")
    add_deps("xsl_asio")
end

target("udp_echo")
do
    set_kind("binary")
    add_files("udp_echo.cpp")
    add_deps("xsl_asio")
end

target("asio_example")
do
    set_kind("binary")
    add_files("asio_example.cpp")
    add_packages("asio")
end

includes("dns")
