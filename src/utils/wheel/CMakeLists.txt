
set(XSL_WHEEL_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "XSL_WHEEL_DIR")

message(STATUS "[SETUP] xsl_wheel: XSL_WHEEL_DIR = ${XSL_WHEEL_DIR}")

add_library(xsl_wheel STATIC)

target_include_directories(xsl_wheel PRIVATE
    ${XSL_WHEEL_DIR}
)
target_sources(xsl_wheel PRIVATE
    ${XSL_WHEEL_DIR}/wheel.cpp
)

target_link_libraries(xsl_wheel
    PRIVATE
    absl::log
)