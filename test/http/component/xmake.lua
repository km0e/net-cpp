target("http_component_static_test")
do
    set_kind("binary")
    set_default(false)
    add_files("test_static.cpp")
    add_tests("http_component_static_test")
    on_package(function(package) end)
end
