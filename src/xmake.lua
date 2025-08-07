add_deps("prepare", { public = true })

includes("net", "wheel", "coro", "sys", "asio", "io")

target("xsl")
do
    set_kind("static")
    add_files("**.cpp")
    add_packages("openssl3", { public = true })
end
