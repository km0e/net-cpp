target("unitest_xsl_feature_flags")
do
    set_kind("binary")
    set_default(false)
    add_deps("w_xtest")
    add_files("*.cpp")
    add_tests("_", { group = "feature" })
end
