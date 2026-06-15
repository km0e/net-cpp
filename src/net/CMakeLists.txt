add_library(xsl_net STATIC)

target_sources(
    xsl_net
    PRIVATE
    uri.cpp
    dns/utils.cpp
    http/proto/accept.cpp
    http/proto/media-type.cpp
    http/request/target.cpp
)

target_link_libraries(xsl_net PUBLIC xsl_log xsl_sys xsl_wheel)
