includes("coro")

target("tcp_echo")do
    set_kind("binary")
    add_files("tcp_echo.cpp")
    add_deps("xsl")
    add_packages("cli11")
end

target("http_server")do
    set_kind("binary")
    add_files("http_server.cpp")
    add_deps("xsl")
    add_packages("cli11")
end


target("udp_client")do
    set_kind("binary")
    add_files("udp_client.cpp")
    add_deps("xsl")
    add_packages("cli11")
end


target("udp_echo")do
    set_kind("binary")
    add_files("udp_echo.cpp")
    add_deps("xsl")
    add_packages("cli11")
end


target("dns_lookup")do
    set_kind("binary")
    add_files("dns_lookup.cpp")
    add_deps("xsl")
    add_packages("cli11")
end
