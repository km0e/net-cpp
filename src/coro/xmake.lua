target("xsl_coro")do
    set_kind("static")
    set_default(false)
    add_files("**.cpp")
    add_deps("xsl_log_ctl")
    on_package(function(package) end)
end
