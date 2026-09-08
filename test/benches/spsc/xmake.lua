add_includedirs("$(projectdir)/include", { public = true })
add_requires("boost")

target("bench_spsc")
do
    set_kind("binary")
    set_default(false)
    add_files("bench_spsc.cpp")
    add_packages("benchmark")
    add_deps("xsl_log")
    set_group("benchmarks/spsc")
end

target("bench_moodycamel_spsc")
do
    set_kind("binary")
    set_default(false)
    add_files("moodycamel/spsc.cpp")
    add_packages("benchmark")
    add_deps("xsl_log")
    set_group("benchmarks/spsc")
end

target("bench_boost_spsc")
do
    set_kind("binary")
    set_default(false)
    add_files("boost/spsc.cpp")
    add_packages("benchmark", "boost")
    add_deps("xsl_log")
    set_group("benchmarks/spsc")
end
