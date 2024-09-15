
for _, file in ipairs(os.files("test_*.cpp")) do
    local name = path.basename(file)
    target(name)
        set_kind("binary")
        set_default(false)
        add_files(name .. ".cpp")
        add_deps("xsl_coro")
        add_packages("gtest","cli11")
        add_tests(name,{group = "coro"})
        add_tests("stable",{runargs = {"-c", "1000"} ,group = "stable"})
        on_package(function(package) end)
end
