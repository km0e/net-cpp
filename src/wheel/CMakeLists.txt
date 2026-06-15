add_library(xsl_wheel STATIC)

target_sources(
    xsl_wheel
    PRIVATE
    str.cpp
    utils.cpp
)

target_link_libraries(xsl_wheel PUBLIC xsl_log)
