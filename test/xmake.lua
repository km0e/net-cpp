set_config("test_scripts_dir", "test/script")

add_packages("gtest")
add_deps("xsl_log_ctl")

includes("compile")

includes("unit")

includes("benches")

includes("integration")
