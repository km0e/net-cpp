
for _, file in ipairs(os.files("*.cpp")) do
    local name = path.basename(file)
    target("unitest_" .. name)
        set_kind("binary")
        set_default(false)
        add_files(name .. ".cpp")
        add_deps("xsl_coro")
        add_packages("cli11")
        add_tests("_",{group = "coro"})
        -- add_tests("stable",{runargs = {"-c", "1000"} ,group = "stable"})
        on_package(function(package) end)
end
