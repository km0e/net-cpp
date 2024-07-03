target("xsl_sync")do
    set_kind("static")
    set_default(false)
    add_files("**.cpp")
    -- after_load(open_log)
    add_deps("xsl_log_ctl")
    on_package(function(package) end)
end
