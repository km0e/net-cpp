includes("http_server", "proto")
-- includes("component")

for _, file in ipairs(os.files("test_*.cpp")) do
    local name = path.basename(file)
    target(name)
    do
        set_kind("binary")
        set_default(false)
        add_files(name .. ".cpp")
        add_deps("w_xtest")
        add_deps("xsl_http")
        add_tests("http" .. name, { group = "http" })
    end
end
