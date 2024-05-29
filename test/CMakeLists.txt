add_library(xsl_for_test STATIC)

target_compile_definitions(xsl_for_test
    PRIVATE
    SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE
)

file(GLOB_RECURSE XSL_SOURCE_FILES
    ${PROJECT_SOURCE_DIR}/source/*.cpp
)

target_sources(xsl_for_test
    PRIVATE
    ${XSL_SOURCE_FILES}
)

target_include_directories(
    xsl_for_test
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

# ---- Options ----
option(ENABLE_TEST_COVERAGE "Enable test coverage" ON)

# ---- code coverage ----
if(ENABLE_TEST_COVERAGE)
    message("Enabling test coverage")
    add_compile_options(-O0 -g -fprofile-arcs -ftest-coverage)
    add_link_options(-fprofile-arcs -ftest-coverage)
endif()

# ---- Add libraries ----
link_libraries(
    xsl_for_test
)

# ---- Add Subdirectories ----
add_subdirectory(http)
add_subdirectory(sync)
add_subdirectory(transport)
