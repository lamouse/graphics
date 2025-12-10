

find_package(GTest CONFIG REQUIRED)
add_executable(
  hello_test
  a.cpp
  vulkan_test.cpp
  common_test.cpp
)
target_link_libraries(
  hello_test PRIVATE GTest::gtest GTest::gtest_main glfw absl::strings
)
target_link_libraries(
  hello_test PRIVATE render-core
)
target_compile_definitions(hello_test PRIVATE VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1)

if(MINGW)
  target_link_libraries(hello_test PRIVATE "-lstdc++exp")
endif()


target_include_directories(
  hello_test PRIVATE ${CMAKE_SOURCE_DIR}/src
)

# GTest::gmock GTest::gmock_main
include(GoogleTest)
gtest_discover_tests(hello_test)
