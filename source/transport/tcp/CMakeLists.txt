add_library(xsl_tcp STATIC)


set(DIRECT_HEADER_LIST
    conn.h
    helper.h
    server.h
    tcp.h
)

foreach(HEADER ${DIRECT_HEADER_LIST})
    set(full_path ${XSL_INC_DIR}/xsl/transport/tcp/${HEADER})
    message(STATUS "[xsl_transport] Add header file: ${full_path}")
    target_sources(xsl_tcp
        PUBLIC
        ${full_path}
    )
endforeach()

set(DIRECT_SOURCE_LIST
    conn.cpp
    helper.cpp
    server.cpp
    tcp.cpp
)

foreach(SOURCE ${DIRECT_SOURCE_LIST})
    set(full_path ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE})
    message(STATUS "[xsl_transport] Add source file: ${full_path}")
    target_sources(xsl_tcp
        PRIVATE
        ${full_path}
    )
endforeach()

target_link_libraries(xsl_tcp
    PUBLIC
    xsl_utils
    xsl_sync
)
