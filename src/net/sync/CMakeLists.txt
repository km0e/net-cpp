add_library(xsl_sync STATIC)

file(GLOB_RECURSE XSL_SYNC_SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

target_sources(
    xsl_sync
    PRIVATE
    ${XSL_SYNC_SOURCE_FILES}
)
