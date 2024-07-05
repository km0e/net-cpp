add_deps("xsl_log_ctl")
add_includedirs("$(projectdir)/test/include", { public = true })

if is_mode("coverage") then
    add_cxxflags("-O0", "-g", "-fprofile-arcs", "-ftest-coverage")
    add_ldflags("-fprofile-arcs", "-ftest-coverage")
end

includes("http", "transport", "feature", "coro", "convert")
