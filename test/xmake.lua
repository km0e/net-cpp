add_packages("spdlog", "fmt", "gtest")

if is_mode("coverage") then
    add_cxxflags("-O0", "-g", "-fprofile-arcs", "-ftest-coverage")
    add_ldflags("-fprofile-arcs", "-ftest-coverage")
end

includes("http", "transport", "wheel")
