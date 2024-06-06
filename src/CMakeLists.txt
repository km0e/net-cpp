add_library(xsl STATIC)

file(GLOB_RECURSE XSL_SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

target_sources(
    xsl
    PRIVATE
    ${XSL_SOURCE_FILES}
)

target_include_directories(
    xsl
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)
