for _, file in ipairs(os.files("**.cpp")) do
    -- eg: request/target.cpp -> request_target
    local name = file:gsub("\\", "_"):gsub("/", "_"):gsub("%.cpp$", "")
    local test_name = "test_http_" .. name
    target(test_name)
    do
        set_kind("binary")
        set_default(false)
        add_files(file)
        add_deps("xsl_asio", "xsl_test_helpers")
        add_tests(test_name, { group = "http" })
    end
end
