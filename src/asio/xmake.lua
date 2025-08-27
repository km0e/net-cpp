target("xsl_asio")
do
    set_kind("static")
    set_default(false)
    add_files("**.cpp")
    add_deps("prepare", { public = true }, "xsl_wheel", "xsl_net", "xsl_io", "xsl_coro")
end
