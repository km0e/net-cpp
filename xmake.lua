set_project("xsl")
set_xmakever("2.5.1")
set_version("0.1.0", { build = "%Y%m%d%H%M" })

-- add release , debug and coverage modes
add_rules("mode.debug", "mode.release", "mode.coverage", "mode.valgrind")

add_rules("plugin.compile_commands.autoupdate", { outputdir = "build" })

set_warnings("everything")
-- set_warnings("all", "error", 'pedantic', 'extra')

set_languages("cxxlatest")

-- dependency
add_requires("toml++", {configs = {header_only = true}})

add_requires("thread-pool", "cli11", "gtest", "quill")

-- log level
option("log")
    set_showmenu(true)
    set_default("trace")
    set_values("none", "trace", "debug", "info", "warn", "error", "critical")
    set_description("Set the log level")
option_end()

add_packages("quill")

function open_log(target)
    local log_level = get_config("log")
    local log_level_map = {
        none = "QUILL_COMPILE_ACTIVE_LOG_LEVEL=-1",
        trace = "QUILL_COMPILE_ACTIVE_LOG_LEVEL=QUILL_COMPILE_ACTIVE_LOG_LEVEL_TRACE_L3",
        debug = "QUILL_COMPILE_ACTIVE_LOG_LEVEL=QUILL_COMPILE_ACTIVE_LOG_LEVEL_DEBUG",
        info = "QUILL_COMPILE_ACTIVE_LOG_LEVEL=QUILL_COMPILE_ACTIVE_LOG_LEVEL_INFO",
        warn = "QUILL_COMPILE_ACTIVE_LOG_LEVEL=QUILL_COMPILE_ACTIVE_LOG_LEVEL_WARNING",
        error = "QUILL_COMPILE_ACTIVE_LOG_LEVEL=QUILL_COMPILE_ACTIVE_LOG_LEVEL_ERROR",
        critical = "QUILL_COMPILE_ACTIVE_LOG_LEVEL=QUILL_COMPILE_ACTIVE_LOG_LEVEL_CRITICAL"
    }
    target:add("defines", log_level_map[log_level])
end

-- flags
add_ldflags("-fuse-ld=mold")

add_includedirs("$(projectdir)/include", { public = true })

xsl_headers = "$(projectdir)/include/(**.h)"

includes("src", "test")
