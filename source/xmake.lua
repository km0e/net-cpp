target("xsl")
do
    set_kind("static")
    add_files(xsl_sources)
    add_headerfiles(xsl_headers)
    -- add_defines("SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG")
    on_load(function(target)
        if get_config("log") == "none" then
            target:add("defines", "SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_OFF")
        elseif get_config("log") == "trace" then
            target:add("defines", "SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE")
        elseif get_config("log") == "debug" then
            target:add("defines", "SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG")
        elseif get_config("log") == "info" then
            target:add("defines", "SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO")
        elseif get_config("log") == "warn" then
            target:add("defines", "SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_WARN")
        elseif get_config("log") == "error" then
            target:add("defines", "SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_ERROR")
        elseif get_config("log") == "critical" then
            target:add("defines", "SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_CRITICAL")
        end
    end)
end
