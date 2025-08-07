for _, file in ipairs(os.files("**.cpp")) do
    -- eg: request/target.cpp -> request_target
    local name = file:gsub("\\", "_"):gsub("/", "_"):gsub("%.cpp$", "")
    local test_name = "test_net_" .. name
    target(test_name)
    do
        set_kind("binary")
        set_default(false)
        add_files(file)
        add_deps("xsl_net")
        add_tests(test_name, { group = "net" })
    end
end
