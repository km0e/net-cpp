for _, file in ipairs(os.files("*.cpp")) do
    local name = path.basename(file)

    target("unitest_" .. name)
    do
        set_kind("binary")
        set_default(false)
        add_files(name .. ".cpp")
        add_deps("w_cli_test")
        add_deps("xsl_coro")
        add_tests("_", { group = "coro" })
        -- add_tests("stable",{runargs = {"-c", "1000"} ,group = "stable"})
    end
end
