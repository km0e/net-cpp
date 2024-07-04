add_packages("gtest")
target("test_xsl_wheel_type_traits")
    set_kind("binary")
    set_default(false)
    add_files("*.cpp")

add_tests("test_xsl_wheel_type_traits")
