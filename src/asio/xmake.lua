target("xsl_asio")
do
    set_kind("static")
    set_default(false)
    add_files("**.cpp")
    add_deps("xsl_sys", "xsl_convert", "xsl_wheel", "xsl_net", "xsl_io", "xsl_coro")
    on_package(function(package) end)
end
