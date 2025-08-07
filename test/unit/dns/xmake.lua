for _, file in ipairs(os.files("*.cpp")) do
    local name = path.basename(file)
    target("ut_" .. name)
    do
        set_kind("binary")
        set_default(false)
        add_files(name .. ".cpp")
        add_deps("xsl_net") -- TODO: change dp
        add_tests("_", { group = "dns" })
        on_package(function(package) end)
    end
end
