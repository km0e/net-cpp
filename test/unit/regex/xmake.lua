add_packages("gtest")
target("test_xsl_regex")
    set_kind("binary")
    set_default(false)
    add_files("*.cpp")
    add_tests("test_xsl_regex")

