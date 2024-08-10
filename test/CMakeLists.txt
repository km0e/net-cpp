# ---- Options ----
option(ENABLE_TEST_COVERAGE "Enable test coverage" ON)

# ---- code coverage ----
if(ENABLE_TEST_COVERAGE)
    message("Enabling test coverage")
    add_compile_options(-O0 -g -fprofile-arcs -ftest-coverage)
    add_link_options(-fprofile-arcs -ftest-coverage)
endif()

# ---- Add libraries ----
link_libraries(xsl_log_ctl)
link_libraries(testlib)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
# ---- Add Subdirectories ----
add_subdirectory(coro)
add_subdirectory(http)
add_subdirectory(transport)
add_subdirectory(feature)
add_subdirectory(convert)
add_subdirectory(regex)
add_subdirectory(wheel)
