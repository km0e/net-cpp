add_library(xsl_wheel STATIC)

target_include_directories(xsl_wheel
    PUBLIC
    ${XSL_INC_DIR}
)

set(DIRECT_HEADER_LIST
    wheel.h
)

foreach(HEADER ${DIRECT_HEADER_LIST})
    set(full_path ${XSL_INC_DIR}/xsl/utils/wheel/${HEADER})
    message(STATUS "[xsl_wheel] Adding header ${full_path}")
    target_sources(xsl_wheel
        PUBLIC
        ${full_path}
    )
endforeach()

set(DIRECT_SOURCE_LIST
    wheel.cpp
)

foreach(SOURCE ${DIRECT_SOURCE_LIST})
    set(full_path ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE})
    message(STATUS "[xsl_wheel] Adding source ${full_path}")
    target_sources(xsl_wheel
        PRIVATE
        ${full_path}
    )
endforeach()

target_link_libraries(xsl_wheel
    PUBLIC
    ${SPDLOG_LIBRARIES}
    ${THREAD_POOL_LIBRARIES}
)
