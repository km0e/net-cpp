set_project("xsl")
set_xmakever("2.5.1")
set_version("0.1.0", {build = "%Y%m%d%H%M"})

-- set warning all as error
set_warnings("all", "error")
set_languages("c++20")
add_cxxflags("-Wno-error=deprecated-declarations")

-- add release and debug mode
add_rules("mode.debug", "mode.release")

add_requires("thread-pool", "spdlog", "cli11", "fmt")

add_rules("plugin.compile_commands.autoupdate", {outputdir = "build"})

add_includedirs("$(projectdir)/include")

includes("include", "src", "tests")
