add_deps("xsl_log_ctl")

if is_mode("coverage") then
    add_cxxflags("-O0", "-g", "-fprofile-arcs", "-ftest-coverage")
    add_ldflags("-fprofile-arcs", "-ftest-coverage")
end

includes("http", "transport", "feature", "coro")
