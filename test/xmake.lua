add_deps("xsl")
add_packages("cli11","fmt","xsl")
if is_mode("coverage") then
    add_cxxflags("-O0","-g","-fprofile-arcs","-ftest-coverage")
    add_ldflags("-fprofile-arcs","-ftest-coverage")
end
includes("http","sync","transport")

