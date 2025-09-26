for _, file in ipairs(os.files("*.cpp")) do
    local name = path.basename(file)

    target("ut_coro_signal_" .. name)
    do
        set_kind("binary")
        set_default(false)
        add_files(name .. ".cpp")
        add_deps("cli", "xsl_coro")
        add_tests("_", { group = "coro_signal" })
        -- add_tests("stable",{runargs = {"-c", "1000"} ,group = "stable"})
    end
end
