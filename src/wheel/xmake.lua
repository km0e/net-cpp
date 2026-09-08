target("xsl_wheel")
do
    set_kind("static")
    set_default(false)
    add_files("**.cpp")
    xsl_enable_unity()
    add_deps("xsl_log", { public = true })
end
