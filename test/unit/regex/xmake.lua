target("unitest_xsl_regex")
do
    set_kind("binary")
    set_default(false)
    add_files("*.cpp")
    add_deps("xsl_log", "xsl_test_helpers")
    add_tests("unitest_xsl_regex", { group = "regex" })
end
