include(cmake/CPM.cmake)

CPMAddPackage(
  NAME quill
  GITHUB_REPOSITORY odygrd/quill
  GIT_TAG v10.0.1
  OPTIONS
    "QUILL_BUILD_EXAMPLES OFF"
    "QUILL_BUILD_TESTS OFF"
    "QUILL_BUILD_BENCHMARKS OFF"
    "QUILL_ENABLE_INSTALL OFF"
)
add_library(loglib ALIAS quill)

CPMAddPackage(
  NAME CLI11
  GITHUB_REPOSITORY CLIUtils/CLI11
  GIT_TAG v2.5.0
)
add_library(clilib ALIAS CLI11)

CPMAddPackage(
  NAME googletest
  GITHUB_REPOSITORY google/googletest
  GIT_TAG v1.17.0
  OPTIONS
    "INSTALL_GTEST OFF"
    "gtest_force_shared_crt ON"
)
