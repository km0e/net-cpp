add_library(xsl_asio STATIC)

file(GLOB_RECURSE XSL_ASIO_SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

target_sources(
    xsl_asio
    PRIVATE
    ${XSL_ASIO_SOURCE_FILES}
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
