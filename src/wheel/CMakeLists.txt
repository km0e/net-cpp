add_library(xsl_wheel STATIC)

file(GLOB_RECURSE XSL_WHEEL_SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

target_sources(
    xsl_wheel
    PRIVATE
    ${XSL_WHEEL_SOURCE_FILES}
)
