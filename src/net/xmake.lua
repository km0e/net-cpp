target("xsl_net")
do
    set_kind("static")
    set_default(false)
    add_files("**.cpp", "dns/**.cpp")
    xsl_enable_unity()
    add_deps("xsl_log", { public = true }, "xsl_sys", "xsl_wheel")
end
