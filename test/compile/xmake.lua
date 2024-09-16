for _, file in ipairs(os.files("*.cpp")) do
    local name = path.basename(file)
    target("compile_" .. name)
        set_kind("binary")
        set_default(false)
        add_files(name .. ".cpp")
        add_deps("xsl")
        add_tests("_",{build_should_pass = true,group = "compile"})
        on_package(function(package) end)
end
