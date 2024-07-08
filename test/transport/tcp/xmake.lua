add_deps("xsl_tcp")
-- includes("tcp_server")

target("test_tcp_connect")
    set_kind("binary")
    set_default(false)
    add_files("test_connect.cpp")
    add_packages("cli11","gtest")
    on_package(function(package) end)
    on_test(function (target)
        local test_file = target:targetfile();
        local scriptdir = os.scriptdir() .. "/script/test_with_echo_server.py"
        print("cmd: ", "python",scriptdir, "127.0.0.1", 12345, "ncat", "cat", test_file)
        local outputdata,errdata = os.iorunv("python", {scriptdir, "127.0.0.1", 12347, "ncat", "cat", test_file})
        if(outputdata ~= "" or errdata ~= "") then
            print(outputdata)
            print(errdata)
            return false
        end
        return true
    end)
    add_tests("test_tcp_connect",{run_timeout=1000})

target("test_tcp_bind")
    set_kind("binary")
    set_default(false)
    add_files("test_bind.cpp")
    add_packages("cli11","gtest")
    on_package(function(package) end)
    add_tests("test_tcp_bind")

target("test_tcp_listen")
    set_kind("binary")
    set_default(false)
    add_files("test_listen.cpp")
    add_packages("cli11","gtest")
    on_package(function(package) end)
    add_tests("test_tcp_listen")

