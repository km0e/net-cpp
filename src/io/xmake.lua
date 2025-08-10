target("xsl_io")
do
    set_kind("static")
    set_default(false)
    add_files("**.cpp")
    add_deps("xsl_sys", "xsl_wheel")
end
