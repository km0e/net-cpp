add_deps("xsl_log_ctl")

add_requires("brotli")

target("xsl_utils")do
    set_kind("static")
    set_default(false)
    add_packages("brotli")
    add_files("**.cpp")
    on_package(function(package) end)
end
