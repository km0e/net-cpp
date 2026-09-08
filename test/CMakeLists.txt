# ---- Options ----
option(ENABLE_TEST_COVERAGE "Enable test coverage" OFF)

# ---- code coverage ----
if(ENABLE_TEST_COVERAGE)
  message(STATUS "Enabling test coverage")
  add_compile_options(-O0 -g -fprofile-arcs -ftest-coverage)
  add_link_options(-fprofile-arcs -ftest-coverage)
endif()

# ---- Shared test scaffolding ----
# xsl_test_helpers: gtest_main + the shared headers in test/include. Every
# test links this (xsl_add_test()/xsl_add_test_glob() do it automatically).
add_library(xsl_test_helpers INTERFACE)
target_include_directories(xsl_test_helpers INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(xsl_test_helpers INTERFACE GTest::gtest_main)

# ---- Add Subdirectories ----
add_subdirectory(unit)
add_subdirectory(integration)
