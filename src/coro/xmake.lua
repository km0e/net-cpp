target("xsl_coro")do
    set_kind("static")
    set_default(false)
    add_files("**.cpp")
    add_deps("xsl_log_ctl")
    -- after_load(open_log)
    on_package(function(package) end)
end
