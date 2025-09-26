# ---- Options ----
option(ENABLE_TEST_COVERAGE "Enable test coverage" OFF)

# ---- code coverage ----
if(ENABLE_TEST_COVERAGE)
  message(STATUS "Enabling test coverage")
  add_compile_options(-O0 -g -fprofile-arcs -ftest-coverage)
  add_link_options(-fprofile-arcs -ftest-coverage)
endif()

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
# ---- Add Subdirectories ----
add_subdirectory(unit)
