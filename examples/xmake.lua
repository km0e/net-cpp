includes("coro")

add_packages("cli11")

target("tcp_echo")do
    set_kind("binary")
    add_files("tcp_echo.cpp")
    add_deps("xsl")
end

target("udp_client")do
    set_kind("binary")
    add_files("udp_client.cpp")
    add_deps("xsl")
end


target("udp_echo")do
    set_kind("binary")
    add_files("udp_echo.cpp")
    add_deps("xsl")
end
