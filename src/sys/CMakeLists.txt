file(GLOB_RECURSE XSL_SYS_SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

add_library(xsl_sys STATIC)

target_sources(
    xsl_sys
    PRIVATE
    ${XSL_SYS_SOURCE_FILES}
)
