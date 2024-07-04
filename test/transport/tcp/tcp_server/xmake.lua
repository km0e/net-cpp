local test_list ={}

for _, file in ipairs(os.files("test_*.cpp")) do
    local name = path.basename(file)
    target(name)
        set_kind("binary")
        set_default(false)
        add_files(name .. ".cpp")
        add_packages("cli11")
        test_list[#test_list+1] = name
        on_package(function(package) end)
end
target("tcp_server_test")
    set_kind("phony")
    add_deps(test_list, "test_tcp_client")
    on_test(function (target)
        local test_files = {}
        for _, test in ipairs(test_list) do
            test_files[#test_files+1] = target:dep(test):targetfile()
        end
        local scriptdir = os.scriptdir() .. "/script/test_with_echo_client.py"
        local client = target:dep("test_tcp_client"):targetfile()
        print("cmd: ", "python",scriptdir, "127.0.0.1", 12346, client, table.unpack(test_files))
        local outputdata,errdata = os.iorunv("python", {scriptdir, "127.0.0.1", 12346, client, table.unpack(test_files)})
        if(outputdata ~= "" or errdata ~= "") then
            print(outputdata)
            print(errdata)
            return false
        end
        return true
    end)
add_tests("tcp_server_test",{run_timeout=1000})
