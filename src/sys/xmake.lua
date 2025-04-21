add_deps("xsl_log_ctl", { public = true })

target("xsl_sys")
do
    set_kind("static")
    set_default(false)
    add_files("**.cpp")
end
