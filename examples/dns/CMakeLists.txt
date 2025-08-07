find_package(SQLite3)

file(GLOB_RECURSE XSL_DNS_APP_UTILS_SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
add_library(xsl_dns_app_utils STATIC ${XSL_DNS_APP_UTILS_SRC_FILES})
target_include_directories(xsl_dns_app_utils PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(xsl_dns_app_utils xsl_asio clilib SQLite::SQLite3)

add_executable(dns_server "server.cpp")
target_link_libraries(dns_server xsl_dns_app_utils)
