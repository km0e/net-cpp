for _, file in ipairs(os.files("*_example.cpp")) do
    local name = path.basename(file)
    target(name)
        set_kind("binary")
        set_default(false)
        add_files(name .. ".cpp")
        add_deps("xsl_coro")
        set_group("examples")
        on_package(function(package) end)
end
