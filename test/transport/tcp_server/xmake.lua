target("test_tcp_server")
    set_kind("binary")
    add_files("test_tcp_server.cpp")


target("tcp_server_test")
    set_kind("phony")
    add_deps("test_tcp_server", "test_tcp_client")
    on_test(function (target)
        local test0 = target:dep("test_tcp_server"):targetfile()
        print("test0: ", test0)
        local ok,status = os.execv("python $(scriptdir)/script/test_with_echo_client.py", {"127.0.0.1", 12345, test0})
        if not ok then
            return false
        end
        return true
    end)
add_tests("tcp_server_test")
