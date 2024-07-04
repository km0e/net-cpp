target("xsl_sync")do
    set_kind("static")
    set_default(false)
    add_files("**.cpp")
    on_package(function(package) end)
end
