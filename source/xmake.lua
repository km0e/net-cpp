target("xsl")
    set_kind("static")
    add_files("**.cpp")
    add_headerfiles("$(projectdir)/include/(**.h)")
    on_load(function ()
        if "$(log)" == "none" then
        else if "$(log)" == "trace" then
            add_defines("SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE")
        else if "$(log)" == "debug" then
            add_defines("SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG")
        else if "$(log)" == "info" then
            add_defines("SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO")
        else if "$(log)" == "warn" then
            add_defines("SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_WARN")
        else if "$(log)" == "error" then
            add_defines("SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_ERROR")
        else if "$(log)" == "critical" then
            add_defines("SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_CRITICAL")
        end
        end
        end
        end
        end
        end
        end
    end)
