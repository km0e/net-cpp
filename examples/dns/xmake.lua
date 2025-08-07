add_deps("xsl_asio")

-- target("dns_lookup")
-- do
--     set_kind("binary")
--     add_files("lookup.cpp")
--     add_deps("xsl_asio")
-- end

target("xsl_dns_app_utils")
do
    set_kind("static")
    add_files("src/**.cpp")
    add_includedirs("$(projectdir)/examples/dns/include", { public = true })
    add_packages("sqlite3", { public = true })
end

target("dns_server")
do
    set_kind("binary")
    add_files("server.cpp")
    add_deps("xsl_dns_app_utils")
end
