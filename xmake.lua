set_project("xsl")
set_xmakever("2.5.1")
set_version("0.1.0", { build = "%Y%m%d%H%M" })

-- add release , debug and coverage modes
add_rules("mode.debug", "mode.release", "mode.coverage")


set_warnings("everything")
-- set_warnings("all", "error", 'pedantic', 'extra')

set_languages("cxx20")


add_requires("toml++", {configs = {header_only = true}})

add_packages("toml++")

add_requires("thread-pool", "cli11", "fmt", "gtest")

add_requires("spdlog", {system = false, configs = {fmt_external = true}})

-- add_requires("boost")

-- add_requires("abseil")

add_ldflags("-fuse-ld=mold")

add_rules("plugin.compile_commands.autoupdate", { outputdir = "build" })

add_includedirs("$(projectdir)/include", { public = true })

set_config("log", "none")

xsl_sources = "$(projectdir)/source/**.cpp"
xsl_headers = "$(projectdir)/include/(**.h)"
includes("src", "test")
