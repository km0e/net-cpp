target("xsl")
do
    set_kind("static")
    add_files(xsl_sources)
    add_headerfiles(xsl_headers)
    add_options("log")
    on_load(function()
        if "$(log)" == "none" then
            print("none")
        else
            if "$(log)" == "trace" then
                target:add_defines("SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE")
                print("trace")
            else
                if "$(log)" == "debug" then
                    target:add_defines("SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG")
                    print("debug")
                else
                    if "$(log)" == "info" then
                        target:add_defines("SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO")
                        print("info")
                    else
                        if "$(log)" == "warn" then
                            target:add_defines("SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_WARN")
                            print("warn")
                        else
                            if "$(log)" == "error" then
                                target:add_defines("SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_ERROR")
                                print("error")
                            else
                                if "$(log)" == "critical" then
                                    target:add_defines("SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_CRITICAL")
                                    print("critical")
                                end
                            end
                        end
                    end
                end
            end
        end
    end)
end
