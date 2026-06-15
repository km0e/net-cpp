add_library(xsl_coro STATIC)

target_sources(
    xsl_coro
    PRIVATE
    core/executor.cpp
)

target_link_libraries(xsl_coro PUBLIC xsl_log)
