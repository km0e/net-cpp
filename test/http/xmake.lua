add_deps("xsl_http")
add_packages("gtest","fmt")
includes("http_server","component")

for _, file in ipairs(os.files("test_*.cpp")) do
    local name = path.basename(file)
    target(name)
        set_kind("binary")
        set_default(false)
        add_files(name .. ".cpp")
        add_tests("http" .. name)
        add_defines("SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE",{public = true})
        on_package(function(package) end)
end
