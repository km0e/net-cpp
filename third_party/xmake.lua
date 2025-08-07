target("cli")
do
    set_kind("phony")
    set_default(false)
    add_packages("cli11", { public = true })
end
target_end()

add_requires("quill")

target("log")
do
    set_kind("static")
    set_default(false)
    add_deps("config")
    add_files("log.cpp")
    add_packages("quill", { public = true })
    add_options("log_level")
    before_prepare(function(target)
        local log_level = get_config("log_level")
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
        target:add("defines", "QUILL_COMPILE_ACTIVE_LOG_LEVEL=" .. log_level_map[log_level], { public = true })
    end)
end

target("test")
do
    set_kind("phony")
    set_default(false)
    add_packages("gtest", { public = true })
end
