target("http_server_test")
do
    set_kind("binary")
    set_default(false)
    add_files("*.cpp")
    add_defines("SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE", { public = true })
end
