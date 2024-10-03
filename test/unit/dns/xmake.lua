for _, file in ipairs(os.files("*.cpp")) do
    local name = path.basename(file)
    target("unitest_" .. name)
        set_kind("binary")
        set_default(false)
        add_files(name .. ".cpp")
        add_deps("xsl") -- TODO: change dp
        add_tests("_",{group = "dns"})
        on_package(function(package) end)
end
