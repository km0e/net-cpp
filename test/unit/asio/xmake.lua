for _, file in ipairs(os.files("*.cpp")) do
    local name = path.basename(file)
    target("ut_" .. name)
    do
        set_kind("binary")
        set_default(false)
        add_files(name .. ".cpp")
        add_deps("xsl_asio", "xsl_test_helpers")
        add_tests("ut_" .. name, { group = "asio" })
    end
end
