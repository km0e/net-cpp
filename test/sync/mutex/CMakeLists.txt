
include_directories(
    ${XSL_INC_DIR}
)

link_libraries(
    xsl_wheel
    xsl_sync
)

add_executable(try_lock_guard try_lock_guard.cpp)

add_test(NAME try_lock_guard COMMAND try_lock_guard)
