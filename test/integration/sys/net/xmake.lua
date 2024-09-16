target("test_connect",function ()
    set_kind("binary")
    set_default(false)
    add_files("test_connect.cpp")
    add_packages("cli11")
    on_package(function(package) end)
    on_test(function (target)
        local test_file = target:targetfile();
        local script = get_config("test_scripts_dir") .. "/with_echo_server.py"
        print("use test script: ", script)
        try
        {
            function ()
                local _, errdata = os.iorunv("python", {script, test_file})
                if (errdata ~= "") then
                    print(errdata)
                    return false
                end
            end,
            catch
            {
                function (errors)
                    print(errors)
                    return false
                end
            }
        }
        return true
    end)
    add_tests("_",{run_timeout=1000})
end)

target("test_bind",function ()
    set_kind("binary")
    set_default(false)
    add_files("test_bind.cpp")
    add_packages("cli11")
    on_package(function(package) end)
    add_tests("_",{run_timeout=1000})
end)
