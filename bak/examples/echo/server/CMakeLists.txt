


add_executable(echo-server)

target_link_libraries(echo-server
    xsl_wheel
    xsl_transport
)

target_include_directories(echo-server
    PRIVATE
    ${XSL_WHEEL_DIR}
    ${XSL_TRANSPORT_DIR}
    abseil::absl
)

target_sources(echo-server
    PRIVATE
    server.cpp
)