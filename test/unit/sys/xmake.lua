for _, file in ipairs(os.files("*.cpp")) do
    local name = path.basename(file)
    target("unitest_" .. name)
    do
        set_kind("binary")
        set_default(false)
        add_files(name .. ".cpp")
        add_deps("xsl_sys")
        add_deps("w_xtest")
        add_tests("_", { group = "sys" })
    end
end
