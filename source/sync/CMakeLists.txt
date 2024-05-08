add_library(xsl_sync STATIC)

set(DIRECT_HEADER_LIST
    poller.h
    mutex.h
)
set(HEADER_LIST)

foreach(HEADER ${DIRECT_HEADER_LIST})
    set(full_path ${XSL_INC_DIR}/xsl/sync/${HEADER})
    message(STATUS "[xsl_sync] Adding header: ${full_path}")
    target_sources(xsl_sync
        PUBLIC
        ${full_path}
    )
endforeach()

set(DIRECT_SOURCE_LIST
    poller.cpp
    mutex.cpp
)
set(SOURCE_LIST)

foreach(SOURCE ${DIRECT_SOURCE_LIST})
    set(full_path ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE})
    message(STATUS "[xsl_sync] Adding source: ${full_path}")
    target_sources(xsl_sync
        PRIVATE
        ${full_path}
    )
endforeach()

target_link_libraries(xsl_sync
    PUBLIC
    xsl_wheel
)
