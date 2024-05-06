add_library(xsl_transport STATIC)
target_include_directories(xsl_transport
    PUBLIC
    ${XSL_INC_DIR}
)

set(DIRECT_HEADER_LIST
    transport.h
    tcp_server.h
    utils.h
)

foreach(HEADER ${DIRECT_HEADER_LIST})
    set(full_path ${XSL_INC_DIR}/xsl/transport/${HEADER})
    message(STATUS "[xsl_transport] Add header file: ${full_path}")
    target_sources(xsl_transport
        PUBLIC
        ${full_path}
    )
endforeach()

set(DIRECT_SOURCE_LIST
    transport.cpp
    tcp_server.cpp
    utils.cpp
)

foreach(SOURCE ${DIRECT_SOURCE_LIST})
    set(full_path ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE})
    message(STATUS "[xsl_transport] Add source file: ${full_path}")
    target_sources(xsl_transport
        PRIVATE
        ${full_path}
    )
endforeach()

target_link_libraries(xsl_transport
    PUBLIC
    xsl_utils
    xsl_sync
)
