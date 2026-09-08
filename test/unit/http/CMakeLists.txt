xsl_add_test_glob(LIBS xsl_asio)

# Nested test files, one executable each; the executable name flattens the
# path (request/line.cpp -> test_http_request_line), matching the xmake side.
file(GLOB_RECURSE _http_nested RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
foreach(_rel IN LISTS _http_nested)
  if(NOT _rel MATCHES "/")
    continue()  # top-level files are handled by xsl_add_test_glob above
  endif()
  string(REPLACE "/" "_" _name "test_http_${_rel}")
  string(REGEX REPLACE "\\.cpp$" "" _name "${_name}")
  xsl_add_test(NAME "${_name}" SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${_rel}" LIBS xsl_asio)
endforeach()
