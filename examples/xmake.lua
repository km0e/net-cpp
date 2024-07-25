target("echo_server")do
    set_kind("binary")
    add_files("echo_server.cpp")
    add_deps("xsl")
    add_packages("cli11")
end

target("http_server")do
    set_kind("binary")
    add_files("http_server.cpp")
    add_deps("xsl")
    add_packages("cli11")
end
