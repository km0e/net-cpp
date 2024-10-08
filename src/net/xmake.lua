add_deps("xsl_log_ctl")

target("xsl_net")do
    set_kind("static")
    set_default(false)
    add_files("**.cpp")
    add_deps("xsl_sys","xsl_convert","xsl_wheel")
    on_package(function(package) end)
end
