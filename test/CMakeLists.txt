# ---- Options ----
option(ENABLE_TEST_COVERAGE "Enable test coverage" ON)

# ---- code coverage ----
if(ENABLE_TEST_COVERAGE)
    message("Enabling test coverage")
    add_compile_options(-O0 -g -fprofile-arcs -ftest-coverage)
    add_link_options(-fprofile-arcs -ftest-coverage)
endif()

# ---- Add Subdirectories ----
add_subdirectory(http)
add_subdirectory(sync)
add_subdirectory(transport)
