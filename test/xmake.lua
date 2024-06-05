target("xsl_for_test")
do
    set_kind("static")
    add_defines("SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE")
    add_files(xsl_sources)
end

add_packages("cli11", "fmt", "gtest")

if is_mode("coverage") then
    add_cxxflags("-O0", "-g", "-fprofile-arcs", "-ftest-coverage")
    add_ldflags("-fprofile-arcs", "-ftest-coverage")
end

includes("http", "sync", "transport", "wheel")
