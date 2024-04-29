
set(XSL_TRANSPORT_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "XSL_TRANSPORT_DIR")

message(STATUS "[SETUP] xsl_transport: XSL_TRANSPORT_DIR = ${XSL_TRANSPORT_DIR}")

add_library(xsl_transport STATIC)

message(STATUS "[DEPENDENCY] xsl_transport: xsl_wheel = ${XSL_WHEEL_DIR}")
target_include_directories(xsl_transport PUBLIC ${XSL_WHEEL_DIR})
message(STATUS "[DEPENDENCY] xsl_transport: absl::log = ${ABSL_INCLUDE_DIR}")
target_include_directories(xsl_transport PUBLIC ${XSL_TRANSPORT_DIR})

target_sources(xsl_transport
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/transport.h
    ${CMAKE_CURRENT_SOURCE_DIR}/tcp_server.h
    ${CMAKE_CURRENT_SOURCE_DIR}/tcp_client.h
    ${CMAKE_CURRENT_SOURCE_DIR}/poller.h
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/transport.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tcp_server.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tcp_client.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/poller.cpp
)

target_link_libraries(xsl_transport
    PUBLIC
    xsl_wheel
    absl::log
)

