add_library(xsl_io STATIC)

file(GLOB_RECURSE XSL_IO_SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

target_sources(
    xsl_io
    PRIVATE
    ${XSL_IO_SOURCE_FILES}
)

