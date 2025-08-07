set_config("test_scripts_dir", "test/script")

target("prepare_t")
do
    set_kind("phony")
    add_deps("prepare", { public = true }, "test")
    add_includedirs("$(projectdir)/test/include", { public = true })
end

includes("compile")

includes("unit")

includes("benches")

includes("integration")
