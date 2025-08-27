set_project("xsl")
set_xmakever("3.0.0")
set_version("0.1.0", { build = "%Y%m%d%H%M" })

-- add release , debug and coverage modes
add_rules(
    "mode.debug",
    "mode.release",
    "mode.coverage",
    "mode.valgrind",
    "plugin.compile_commands.autoupdate",
    { outputdir = "." }
)

set_warnings("everything")
-- set_warnings("all", "error", 'pedantic', 'extra')

-- set_languages("cxx23")
set_languages("cxxlatest")

if is_mode("release") then
    set_optimize("fastest")
end

-- dependency
add_requires("thread-pool", "cli11", "openssl3")

add_requires("asio")

set_policy("build.optimization.lto", true)
-- log level

option("log_level")
do
    set_default("trace")
    set_description("Set the log level for the project.")
    set_values("none", "trace", "debug", "info", "warning", "error", "critical")
    after_check(function(option)
        local log_level = option:value()
        local log_levels = { "none", "trace", "debug", "info", "warning", "error", "critical" }
        local log_level_map = {}
        log_level_map[log_levels[1]] = "8"
        log_level_map[log_levels[2]] = "QUILL_COMPILE_ACTIVE_LOG_LEVEL_TRACE_L1"
        log_level_map[log_levels[3]] = "QUILL_COMPILE_ACTIVE_LOG_LEVEL_DEBUG"
        log_level_map[log_levels[4]] = "QUILL_COMPILE_ACTIVE_LOG_LEVEL_INFO"
        log_level_map[log_levels[5]] = "QUILL_COMPILE_ACTIVE_LOG_LEVEL_WARNING"
        log_level_map[log_levels[6]] = "QUILL_COMPILE_ACTIVE_LOG_LEVEL_ERROR"
        log_level_map[log_levels[7]] = "QUILL_COMPILE_ACTIVE_LOG_LEVEL_CRITICAL"
        print("Setting log level to: " .. log_level)
        option:add("defines", "QUILL_COMPILE_ACTIVE_LOG_LEVEL=" .. log_level_map[log_level], { public = true })
    end)
end

includes("third_party")

target("prepare")
do
    set_kind("static")
    set_default(false)
    add_files("src/log.cpp")
    add_includedirs("$(projectdir)/include", { public = true })
    add_options("log_level", { public = true })
    add_ldflags("-fuse-ld=mold", { force = true })
    add_packages("openssl3", { public = true }, "quill", { public = true })
end

target("xsl")
do
    set_kind("static")
    add_files("src/**.cpp")
    add_headerfiles("$(projectdir)/include/(xsl/**.h)")
    add_includedirs("$(projectdir)/include", { public = true })
    add_options("log_level", { public = true })
    add_ldflags("-fuse-ld=mold", { force = true })
    add_packages("openssl3", { public = true }, "quill", { public = true })
end
-- set_policy("build.sanitizer.thread", true)
-- set_policy("build.sanitizer.address", true)
-- set_policy("build.sanitizer.memory", true)
-- set_policy("build.sanitizer.leak", true)
-- set_policy("build.sanitizer.undefined", true)

includes("src")
includes("test")
includes("examples")
