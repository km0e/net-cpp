set_project("xsl")
set_xmakever("2.5.1")
set_version("0.1.0", {build = "%Y%m%d%H%M"})

-- add release , debug and coverage modes
add_rules("mode.debug", "mode.release", "mode.coverage")


set_warnings("everything")
-- set_warnings("all", "error", 'pedantic', 'extra')

set_languages("c++20")


add_requires("thread-pool", "spdlog", "cli11", "fmt")

-- add_requires("boost")

-- add_requires("abseil")

add_ldflags("-fuse-ld=mold")

add_rules("plugin.compile_commands.autoupdate", {outputdir = "build"})

add_includedirs("$(projectdir)/include", {public = true})

includes("include", "source", "test")
