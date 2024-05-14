includes("http", "sync", "transport", "utils")

target("net")
    set_kind("static")
    add_deps("http", "sync", "transport", "net_utils")
