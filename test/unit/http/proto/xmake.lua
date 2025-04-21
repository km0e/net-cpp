for _, file in ipairs(os.files("test_*.cpp")) do
    local name = path.basename(file)
    target(name)
    set_kind("binary")
    set_default(false)
    add_files(name .. ".cpp")
    add_deps("xsl_http")
    add_deps("w_xtest")
    add_tests(name, { group = "http" })
end
