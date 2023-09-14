cmake_minimum_required(VERSION 3.19)
project(test)

if(WIN32)
  cmake_policy(SET CMP0135 NEW)
  include(FetchContent)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
  )

  # # For Windows: Prevent overriding the parent project's compiler/linker settings
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
else()
  # vcpkg
  find_package(GTest CONFIG REQUIRED)
endif()

add_executable(
  hello_test
  a.cpp
)
target_link_libraries(
  hello_test PRIVATE GTest::gtest GTest::gtest_main
)

# GTest::gmock GTest::gmock_main
include(GoogleTest)
gtest_discover_tests(hello_test)
add_test(AllTestsInMain hello_test)
