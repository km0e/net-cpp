
target("http_server_test")
do
    set_kind("binary")
    set_default(false)
    add_files("*.cpp")
    add_deps("xsl_http")
    add_packages("cli11")
    on_package(function(package) end)
end
