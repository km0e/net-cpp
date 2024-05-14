target("tcp")
    set_kind("static")
    add_files("*.cpp")
    add_deps("sync","net_utils")
