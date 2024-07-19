target("xsl_log_ctl")do
    set_kind("static")
    set_default(false)
    add_files("logctl.cpp")
    add_options("log_level")
    before_build(function(target)
        local log_level = get_config("log_level")
        local log_level_map = {
            none = "QUILL_COMPILE_ACTIVE_LOG_LEVEL=8",
            trace = "QUILL_COMPILE_ACTIVE_LOG_LEVEL=QUILL_COMPILE_ACTIVE_LOG_LEVEL_TRACE_L3",
            debug = "QUILL_COMPILE_ACTIVE_LOG_LEVEL=QUILL_COMPILE_ACTIVE_LOG_LEVEL_DEBUG",
            info = "QUILL_COMPILE_ACTIVE_LOG_LEVEL=QUILL_COMPILE_ACTIVE_LOG_LEVEL_INFO",
            warn = "QUILL_COMPILE_ACTIVE_LOG_LEVEL=QUILL_COMPILE_ACTIVE_LOG_LEVEL_WARNING",
            error = "QUILL_COMPILE_ACTIVE_LOG_LEVEL=QUILL_COMPILE_ACTIVE_LOG_LEVEL_ERROR",
            critical = "QUILL_COMPILE_ACTIVE_LOG_LEVEL=QUILL_COMPILE_ACTIVE_LOG_LEVEL_CRITICAL"
        }
        target:add("defines", log_level_map[log_level],{public = true}) -- public is important
    end)
    on_package(function(package) end)
end

includes("net","utils","wheel","coro","sys")


target("xsl_convert")do
    set_kind("static")
    set_default(false)
    add_files("convert.cpp")
    add_deps("xsl_log_ctl")
    on_package(function(package) end)
end

target("xsl")
do
    set_kind("static")
    add_files("**.cpp")
    add_deps("xsl_log_ctl")
    add_headerfiles(xsl_headers)
end
