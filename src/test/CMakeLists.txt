cmake_minimum_required(VERSION 3.19)
project(test)
# include(FetchContent)
# FetchContent_Declare(
#   googletest
#   URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
# )
# # For Windows: Prevent overriding the parent project's compiler/linker settings
# set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
# FetchContent_MakeAvailable(googletest)

#vcpkg
find_package(GTest CONFIG REQUIRED)

add_executable(
  hello_test
  a.cpp
)
target_link_libraries(
  hello_test PRIVATE GTest::gtest GTest::gtest_main
)
#GTest::gmock GTest::gmock_main

include(GoogleTest)
gtest_discover_tests(hello_test)
add_test(AllTestsInMain hello_test)
