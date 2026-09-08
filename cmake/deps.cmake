include(cmake/CPM.cmake)

CPMAddPackage(
  NAME quill
  GITHUB_REPOSITORY odygrd/quill
  GIT_TAG v10.0.1
  OPTIONS
    "QUILL_BUILD_EXAMPLES OFF"
    "QUILL_BUILD_TESTS OFF"
    "QUILL_BUILD_BENCHMARKS OFF"
    "QUILL_ENABLE_INSTALL ON"
)
add_library(loglib ALIAS quill)

# ---- standalone asio (HTTP server vs asio comparison, see test/benches/http) ----
CPMAddPackage(
  NAME asio
  GITHUB_REPOSITORY chriskohlhoff/asio
  GIT_TAG asio-1-36-0
  SYSTEM YES
  EXCLUDE_FROM_ALL YES
  DOWNLOAD_ONLY YES
)
if(asio_ADDED)
  add_library(asio INTERFACE)
  target_include_directories(asio SYSTEM INTERFACE ${asio_SOURCE_DIR}/asio/include)
  target_compile_definitions(asio INTERFACE ASIO_STANDALONE ASIO_NO_DEPRECATED)
  add_library(asio::asio ALIAS asio)
endif()

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
