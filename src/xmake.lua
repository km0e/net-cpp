target("xsl_log_ctl")
do
    set_kind("static")
    set_default(false)
    add_files("logctl.cpp")
    add_options("log_level")
    before_build(set_log_level)
    on_package(function(package) end)
end

includes("net", "wheel", "coro", "sys")


target("xsl_convert")
do
    set_kind("static")
    set_default(false)
    add_files("convert.cpp")
    add_deps("xsl_log_ctl")
    on_package(function(package) end)
end

target("xsl")
do
    set_kind("static")
    set_options("log_level")
    add_files("**.cpp")
    -- add_deps("xsl_log_ctl")
    before_build(set_log_level)
    add_headerfiles(xsl_headers)
end
