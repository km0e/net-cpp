add_packages("gtest")

for _, file in ipairs(os.files("test_*.cpp")) do
    local name = path.basename(file)
    target(name)
        set_kind("binary")
        set_default(false)
        add_files(name .. ".cpp")
        add_deps("xsl_wheel")
        add_tests(name,{group = "xsl_wheel"})
        on_package(function(package) end)
end
