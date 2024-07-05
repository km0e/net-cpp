set_project("xsl")
set_xmakever("2.5.1")
set_version("0.1.0", { build = "%Y%m%d%H%M" })

-- add release , debug and coverage modes
add_rules("mode.debug", "mode.release", "mode.coverage", "mode.valgrind")

add_rules("plugin.compile_commands.autoupdate", { outputdir = "build" })

set_warnings("everything")
-- set_warnings("all", "error", 'pedantic', 'extra')

set_languages("cxxlatest")

-- for support <expected> 
-- add_defines("__cpp_concepts=202002")

-- dependency
add_requires("toml++", {configs = {header_only = true}})

add_requires("thread-pool", "cli11", "gtest", "quill")

-- log level
set_config("log_level", "trace")

add_packages("quill")


-- flags
add_ldflags("-fuse-ld=mold")

add_includedirs("$(projectdir)/include", { public = true })

xsl_headers = "$(projectdir)/include/(**.h)"

includes("src", "test")
