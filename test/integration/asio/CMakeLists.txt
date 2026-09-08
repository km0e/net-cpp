# Self-contained integration test (creates its own TCP server)
xsl_add_test(NAME it_bind SOURCES bind.cpp LIBS xsl_asio clilib TIMEOUT 60)

# it_connect runs against a Python TCP+UDP echo server, which allocates a
# free port and passes it to the test via -i/-p (see test/script/).
find_package(Python3 QUIET)
if(Python3_FOUND)
  xsl_add_test(
    NAME it_connect
    SOURCES connect.cpp
    LIBS xsl_asio clilib
    TIMEOUT 60
    PREFIX "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/test/script/with_echo_server.py"
  )
else()
  message(STATUS "Python3 not found: it_connect integration test disabled")
endif()
