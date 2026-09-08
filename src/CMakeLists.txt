
add_subdirectory(sys)
add_subdirectory(net)
add_subdirectory(coro)
add_subdirectory(wheel)
add_subdirectory(asio)

add_library(xsl STATIC)

target_sources(
    xsl
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/xsl.cpp
)

target_include_directories(xsl PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(xsl PUBLIC xsl_net xsl_coro xsl_sys xsl_wheel xsl_asio)
