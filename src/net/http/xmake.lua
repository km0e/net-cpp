target("xsl_http")do
    set_kind("static")
    set_default(false)
    add_files("**.cpp")
    add_deps("xsl_tcp","xsl_convert")
    add_defines("SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE",{public = true})
    on_package(function(package) end)
end
