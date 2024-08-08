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

option("log_level")
    set_showmenu(true)
    set_default("info")
    set_values("trace", "debug", "info", "warning", "error", "critical", "none")
    set_description("Set the log level")
option_end()

function set_log_level(target)
    local log_level = get_config("log_level")
    local log_levels = {"none", "trace", "debug", "info", "warning", "error", "critical"}
    local log_level_map = {}
    log_level_map[log_levels[1]] = "QUILL_COMPILE_ACTIVE_LOG_LEVEL=8"
    log_level_map[log_levels[2]] = "QUILL_COMPILE_ACTIVE_LOG_LEVEL=QUILL_COMPILE_ACTIVE_LOG_LEVEL_TRACE_L3"
    log_level_map[log_levels[3]] = "QUILL_COMPILE_ACTIVE_LOG_LEVEL=QUILL_COMPILE_ACTIVE_LOG_LEVEL_DEBUG"
    log_level_map[log_levels[4]] = "QUILL_COMPILE_ACTIVE_LOG_LEVEL=QUILL_COMPILE_ACTIVE_LOG_LEVEL_INFO"
    log_level_map[log_levels[5]] = "QUILL_COMPILE_ACTIVE_LOG_LEVEL=QUILL_COMPILE_ACTIVE_LOG_LEVEL_WARNING"
    log_level_map[log_levels[6]] = "QUILL_COMPILE_ACTIVE_LOG_LEVEL=QUILL_COMPILE_ACTIVE_LOG_LEVEL_ERROR"
    log_level_map[log_levels[7]] = "QUILL_COMPILE_ACTIVE_LOG_LEVEL=QUILL_COMPILE_ACTIVE_LOG_LEVEL_CRITICAL"
    target:add("defines", log_level_map[log_level], { public = true }) -- public is important
    print("log define: ", log_level_map[log_level])
    print("log define: ", target:get("defines"))
end

add_packages("quill")


-- flags
add_ldflags("-fuse-ld=mold")

add_includedirs("$(projectdir)/include", { public = true })

xsl_headers = "$(projectdir)/include/(**.h)"

includes("src", "test", "examples")
