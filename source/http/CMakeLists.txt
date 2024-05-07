add_library(xsl_http STATIC)
target_include_directories(xsl_http
    PUBLIC
    ${XSL_INC_DIR}
)

set(DIRECT_HEADER_LIST
    http.h
    server.h
    router.h
    msg.h
)

foreach(HEADER ${DIRECT_HEADER_LIST})
    set(full_path ${XSL_INC_DIR}/xsl/http/${HEADER})
    message(STATUS "[xsl_transport] Add header file: ${full_path}")
    target_sources(xsl_http
        PUBLIC
        ${full_path}
    )
endforeach()

set(DIRECT_SOURCE_LIST
    server.cpp
    router.cpp
    msg.cpp
)

foreach(SOURCE ${DIRECT_SOURCE_LIST})
    set(full_path ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE})
    message(STATUS "[xsl_transport] Add source file: ${full_path}")
    target_sources(xsl_http
        PRIVATE
        ${full_path}
    )
endforeach()

target_link_libraries(xsl_http
    PUBLIC
    xsl_transport
    xsl_utils
    xsl_sync
    ${SPDLOG_LIBRARIES}
)
