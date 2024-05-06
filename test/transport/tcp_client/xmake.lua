target("test_tcp_client")
    set_kind("binary")
    add_files("test_tcp_client.cpp")
    set_default(false)
target("test_tcp_client_with_poll")
    set_kind("binary")
    add_files("test_tcp_client_with_poll.cpp")
    set_default(false)
target("tcp_client_test")
    set_kind("phony")
    add_deps("test_tcp_client", "test_tcp_client_with_poll")
    on_test(function (target)
        local test0 = target:dep("test_tcp_client"):targetfile()
        print("test0: ", test0)
        local test1 = target:dep("test_tcp_client_with_poll"):targetfile()
        print("test1: ", test1)
        local ok,status = os.execv("python $(scriptdir)/script/test_with_echo_server.py", {"127.0.0.1", 12345, "ncat", "cat", test0, test1})
        if not ok then
            return false
        end
        return true
    end)
add_tests("tcp_client_test")
