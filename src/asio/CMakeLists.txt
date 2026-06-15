add_library(xsl_asio STATIC)

target_sources(
    xsl_asio
    PRIVATE
    http.cpp
    pipe.cpp
    http/request.cpp
    http/response.cpp
)

target_link_libraries(
    xsl_asio
    PUBLIC
    xsl_log
    xsl_sys
    xsl_net
    xsl_coro
    xsl_wheel
    OpenSSL::SSL
    OpenSSL::Crypto
)
