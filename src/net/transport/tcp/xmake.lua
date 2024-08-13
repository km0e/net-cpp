target("xsl_tcp")do
    set_kind("static")
    set_default(false)
    add_files("**.cpp")
    add_deps("xsl_sync","xsl_coro","xsl_convert","xsl_sys")
    on_package(function(package) end)
end
