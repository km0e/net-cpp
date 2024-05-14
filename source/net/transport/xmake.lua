includes("tcp")

target("transport")
    set_kind("static")
    add_files("*.cpp")
    add_deps("sync","net_utils","tcp")
