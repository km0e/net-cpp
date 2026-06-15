add_library(xsl_sys STATIC)

target_sources(
    xsl_sys
    PRIVATE
    io.cpp
    raw.cpp
    io/context/epoll.cpp
    net/sockaddr.cpp
)

target_link_libraries(xsl_sys PUBLIC xsl_log)
