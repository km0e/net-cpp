set_config("test_scripts_dir", "test/script")

target("w_xtest")
do
    set_kind("phony")
    add_packages("gtest", { public = true })
    add_deps("xsl_log_ctl", { public = true })
    add_includedirs("$(projectdir)/test/include", { public = true })
end

target("w_cli_test")
do
    set_kind("phony")
    add_deps("w_xtest", { public = true })
    add_deps("w_cli", { public = true })
end

includes("compile")

includes("unit")

includes("benches")

includes("integration")
