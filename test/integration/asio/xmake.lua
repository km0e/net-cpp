add_deps("xsl_asio")

target("intest_connect")
do
    set_kind("binary")
    set_default(false)
    add_files("connect.cpp")
    add_deps("w_cli_test")
    on_test(function(target)
        local test_file = target:targetfile()
        local script = get_config("test_scripts_dir") .. "/with_echo_server.py"
        try({
            function()
                print("Running command: python " .. script .. " " .. test_file)
                local _, errdata = os.iorunv("python", { script, test_file })
                if errdata ~= "" then
                    print(errdata)
                    return false
                end
            end,
            catch({
                function(errors)
                    print(errors)
                    return false
                end,
            }),
        })
        return true
    end)
    add_tests("_", { run_timeout = 1000, group = "asio" })
end

target("intest_bind")
do
    set_kind("binary")
    set_default(false)
    add_files("bind.cpp")
    add_deps("w_cli_test")
    add_tests("_", { run_timeout = 1000, group = "asio" })
end
