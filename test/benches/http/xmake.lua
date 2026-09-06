-- HTTP server benchmarks: xsl::asio vs standalone asio
-- Usage: xmake build -g benchmarks/http, see bench.sh

add_requires("asio")

target("bench_http_xsl")
do
    set_kind("binary")
    set_default(false)
    add_files("server_xsl.cpp")
    add_deps("xsl_asio")
    add_includedirs("$(projectdir)/test/include", { public = true })
    set_group("benchmarks/http")
end

target("bench_http_asio")
do
    set_kind("binary")
    set_default(false)
    add_files("server_asio.cpp")
    add_packages("asio")
    add_includedirs("$(projectdir)/test/include", { public = true })
    set_group("benchmarks/http")
end

target("bench_http_loadgen")
do
    set_kind("binary")
    set_default(false)
    add_files("loadgen.cpp")
    add_syslinks("pthread")
    set_group("benchmarks/http")
end
