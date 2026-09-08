set_config("test_scripts_dir", "test/script")

-- Shared test scaffolding: gtest + CLI11 + the headers in test/include.
-- (Replaces the old prepare_t -> prepare/test/cli phony-target chain.)
target("xsl_test_helpers")
do
    set_kind("phony")
    set_default(false)
    add_packages("gtest", { public = true }, "cli11", { public = true })
    add_includedirs("$(projectdir)/test/include", { public = true })
end

includes("unit")

includes("benches")

includes("integration")
