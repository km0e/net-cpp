set_project("xsl")
set_xmakever("3.0.0")
set_version("0.1.0", { build = "%Y%m%d%H%M" })

-- add release , debug and coverage modes
add_rules("mode.debug", "mode.release", "mode.coverage", "mode.valgrind")
add_rules("plugin.compile_commands.autoupdate", { outputdir = "." })

set_warnings("everything")
-- set_warnings("all", "error", 'pedantic', 'extra')

set_languages("cxx23")

-- dependency
add_requires("toml++[header_only]", "thread-pool", "cli11", "openssl3", "sqlite3")

add_requires("asio")

set_policy("build.optimization.lto", true)
-- log level

option("log_level")
do
    set_default("trace")
    set_description("Set the log level for the project.")
    set_values("none", "trace", "debug", "info", "warning", "error", "critical")
end

target("config")
do
    set_kind("phony")
    set_default(false)
    add_includedirs("$(projectdir)/include", { public = true })
    add_ldflags("-fuse-ld=mold", { force = true })
end

includes("third_party")

-- set_policy("build.sanitizer.thread", true)
-- set_policy("build.sanitizer.address", true)
-- set_policy("build.sanitizer.memory", true)
-- set_policy("build.sanitizer.leak", true)
-- set_policy("build.sanitizer.undefined", true)

target("prepare")
do
    set_kind("phony")
    set_default(false)
    add_deps("config", { public = true }, "log", { public = true })
end

includes("src")
includes("test")

includes("examples")
