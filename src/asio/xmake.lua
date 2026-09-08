target("xsl_asio")
do
    set_kind("static")
    set_default(false)
    add_files("**.cpp")
    xsl_enable_unity()
    add_deps("xsl_log", { public = true }, "xsl_wheel", "xsl_net", "xsl_coro")
    add_packages("openssl3", { public = true })
end
