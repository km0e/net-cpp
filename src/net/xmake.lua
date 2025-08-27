target("xsl_net")
do
    set_kind("static")
    set_default(false)
    add_files("**.cpp", "dns/**.cpp")
    add_deps("prepare", { public = true }, "xsl_sys", "xsl_wheel")
end
