


add_executable(echo-server)

target_link_libraries(echo-server
    ${FMT_LIBRARIES}
)

target_include_directories(echo-server
    PRIVATE
    abseil::absl
)

target_sources(echo-server
    PRIVATE
    server.cpp
)
