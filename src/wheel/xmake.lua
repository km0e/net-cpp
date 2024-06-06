target("xsl_wheel")do
    set_kind("static")
    set_default(false)
    add_files("**.cpp")
    add_defines("SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE",{public = true})
    on_package(function(package) end)
end
