
add_subdirectory(wheel)

add_library(xsl_utils STATIC)

set(DIRECT_HEADER_LIST
    utils.h
)

foreach(HEADER ${DIRECT_HEADER_LIST})
    set(full_path ${XSL_INC_DIR}/xsl/utils/${HEADER})
    message(STATUS "[xsl_wheel] Adding header ${full_path}")
    target_sources(xsl_utils
        PUBLIC
        ${full_path}
    )
endforeach()

set(DIRECT_SOURCE_LIST
    utils.cpp
)

foreach(SOURCE ${DIRECT_SOURCE_LIST})
    set(full_path ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE})
    message(STATUS "[xsl_wheel] Adding source ${full_path}")
    target_sources(xsl_utils
        PRIVATE
        ${full_path}
    )
endforeach()

target_link_libraries(xsl_utils
    PUBLIC
    ${SPDLOG_LIBRARIES}
    ${THREAD_POOL_LIBRARIES}
)
