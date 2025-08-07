add_deps("prepare_t", { public = true })

add_includedirs("$(projectdir)/test/include", { public = true })

if is_mode("coverage") then
    add_cxxflags("-O0", "-g", "-fprofile-arcs", "-ftest-coverage")
    add_ldflags("-fprofile-arcs", "-ftest-coverage")
end

add_requires("gtest")

includes("http", "feature", "coro", "regex", "wheel", "net")
includes("dns")
includes("sys")
includes("sync")
includes("ser")
