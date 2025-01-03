cmake_minimum_required(VERSION 3.19)
project(test)

#if(WIN32)
#  cmake_policy(SET CMP0077 NEW)
#  include(FetchContent)
#  FetchContent_Declare(
#    googletest
#    GIT_REPOSITORY https://ghproxy.com/https://github.com/google/googletest.git
#    GIT_TAG v1.14.0
#    SOURCE_DIR ${CMAKE_SOURCE_DIR}/thirdparty/gtest/1.14.0
#    FIND_PACKAGE_ARGS NAMES GTest
#  )
#
#  # # For Windows: Prevent overriding the parent project's compiler/linker settings
#  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
#  FetchContent_MakeAvailable(googletest)
#else()
#  # vcpkg
#  find_package(GTest CONFIG REQUIRED)
#endif()
find_package(GTest CONFIG REQUIRED)
add_executable(
  hello_test
  a.cpp
  vulkan_test.cpp
  common_test.cpp
)
target_link_libraries(
  hello_test PRIVATE GTest::gtest GTest::gtest_main
)
target_link_libraries(
  hello_test PRIVATE Vulkan::Vulkan render-core
)
target_include_directories(
  hello_test PRIVATE ${CMAKE_SOURCE_DIR}/src
)
# GTest::gmock GTest::gmock_main
include(GoogleTest)
gtest_discover_tests(hello_test)
add_test(AllTestsInMain hello_test)
