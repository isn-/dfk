add_executable(ut-dfk
  main.c)

target_link_libraries(ut-dfk dfk)
add_test(ut-dfk "${CMAKE_CURRENT_BINARY_DIR}/ut-dfk")
