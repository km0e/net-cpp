add_library(xsl_utils STATIC)

file(GLOB_RECURSE XSL_UTILS_SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

target_sources(
    xsl_utils
    PRIVATE
    ${XSL_UTILS_SOURCE_FILES}
)
