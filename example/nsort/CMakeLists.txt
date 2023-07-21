add_executable(nsort ${CMAKE_CURRENT_SOURCE_DIR}/nsort.c)

target_link_libraries(nsort PRIVATE bque)
