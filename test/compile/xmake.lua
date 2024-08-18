for _, file in ipairs(os.files("test_*.cpp")) do
    local name = path.basename(file)
    target(name)
        set_kind("binary")
        set_default(false)
        add_files(name .. ".cpp")
        add_deps("xsl")
        add_tests(name,{build_should_pass = true,group = "compile"})
        on_package(function(package) end)
end
