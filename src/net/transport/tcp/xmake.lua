target("xsl_tcp")do
    set_kind("static")
    set_default(false)
    add_files("**.cpp")
    add_deps("xsl_utils","xsl_sync","xsl_coro","xsl_convert")
    on_package(function(package) end)
end
