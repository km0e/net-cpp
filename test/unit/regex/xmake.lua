target("unitest_xsl_regex")
do
    set_kind("binary")
    set_default(false)
    add_files("*.cpp")
    add_tests("_", { group = "regex" })
end
