target("xsl_log_ctl")do
    set_kind("static")
    set_default(false)
    add_files("logctl.cpp")
    after_load(open_log)
    on_package(function(package) end)
end

includes("net","utils","wheel","coro")


target("xsl_convert")do
    set_kind("static")
    set_default(false)
    add_files("convert.cpp")
    add_deps("xsl_log_ctl")
    on_package(function(package) end)
end
target("xsl")
do
    set_kind("static")
    add_files("**.cpp")
    add_headerfiles(xsl_headers)
end
