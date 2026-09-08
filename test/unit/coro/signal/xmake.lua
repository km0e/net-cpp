for _, file in ipairs(os.files("*.cpp")) do
    local name = path.basename(file)
    target("ut_coro_signal_" .. name)
    do
        set_kind("binary")
        set_default(false)
        add_files(name .. ".cpp")
        add_deps("xsl_coro", "xsl_test_helpers")
        add_tests("ut_coro_signal_" .. name, { group = "coro_signal" })
    end
end
