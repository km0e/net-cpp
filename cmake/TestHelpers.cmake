# Test registration helpers.
#
# One gtest executable per source file, with automatic labels (the path
# components under test/, e.g. test/unit/coro -> "unit;coro") and a timeout.
# Every test links the shared xsl_test_helpers INTERFACE target defined in
# test/CMakeLists.txt (gtest_main + test/include headers).

# xsl_add_test(NAME <name> [SOURCES <src>...] [LIBS <lib>...]
#              [INCLUDE_DIRS <dir>...] [TIMEOUT <sec>] [PREFIX <cmd>...])
#   PREFIX prefixes the test command, e.g. a Python wrapper script that
#   starts an echo server and then runs the executable.
function(xsl_add_test)
  cmake_parse_arguments(ARG "" "NAME;TIMEOUT" "SOURCES;LIBS;INCLUDE_DIRS;PREFIX" ${ARGN})
  if(NOT ARG_NAME)
    message(FATAL_ERROR "xsl_add_test: NAME is required")
  endif()
  if(NOT ARG_SOURCES)
    set(ARG_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${ARG_NAME}.cpp")
  endif()
  if(NOT ARG_TIMEOUT)
    set(ARG_TIMEOUT 120)
  endif()
  add_executable(${ARG_NAME} ${ARG_SOURCES})
  target_link_libraries(${ARG_NAME} PRIVATE ${ARG_LIBS} xsl_test_helpers)
  foreach(_dir IN LISTS ARG_INCLUDE_DIRS)
    target_include_directories(${ARG_NAME} PRIVATE ${_dir})
  endforeach()
  add_test(NAME ${ARG_NAME} COMMAND ${ARG_PREFIX} $<TARGET_FILE:${ARG_NAME}>)
  file(RELATIVE_PATH _rel "${PROJECT_SOURCE_DIR}/test" "${CMAKE_CURRENT_SOURCE_DIR}")
  string(REPLACE "/" ";" _labels "${_rel}")
  set_tests_properties(${ARG_NAME} PROPERTIES TIMEOUT ${ARG_TIMEOUT} LABELS "${_labels}")
endfunction()

# xsl_add_test_glob([LIBS <lib>...] [TIMEOUT <sec>])
#   Adds one test per *.cpp directly in the current source directory
#   (non-recursive), named after the source file.
function(xsl_add_test_glob)
  cmake_parse_arguments(ARG "" "TIMEOUT" "LIBS" ${ARGN})
  file(GLOB _sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
  foreach(_src IN LISTS _sources)
    get_filename_component(_name "${_src}" NAME_WE)
    xsl_add_test(NAME "${_name}" SOURCES "${_src}" LIBS ${ARG_LIBS} TIMEOUT ${ARG_TIMEOUT})
  endforeach()
endfunction()
