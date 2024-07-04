includes("http_server","component")

add_deps("xsl_http")
add_packages("gtest")

for _, file in ipairs(os.files("test_*.cpp")) do
    local name = path.basename(file)
    target(name)
        set_kind("binary")
        set_default(false)
        add_files(name .. ".cpp")
        add_tests("http" .. name)
        on_package(function(package) end)
end
