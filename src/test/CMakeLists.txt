

find_package(GTest CONFIG REQUIRED)
set(TEST_NAME all_test)


add_executable(
  ${TEST_NAME}
  a.cpp
  vulkan_test.cpp
  common_test.cpp
  resource_test.cpp
)


file(COPY ${CMAKE_SOURCE_DIR}/images/test DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test/image)
target_link_libraries(
  ${TEST_NAME} PRIVATE GTest::gtest GTest::gtest_main absl::strings resource
)
target_link_libraries(
  ${TEST_NAME} PRIVATE render-core
)
target_compile_definitions(${TEST_NAME} PRIVATE VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1)
target_compile_definitions(${TEST_NAME} PRIVATE IMAGE_RESOURCE_PATH="${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
if(TARGET Vulkan::Headers)
    target_link_libraries(${TEST_NAME} PRIVATE Vulkan::Headers)
else()
    target_link_libraries(${TEST_NAME} PRIVATE vulkan-headers::vulkan-headers vulkan-utility-libraries::vulkan-utility-libraries)
endif()

if(MINGW)
  target_link_libraries(${TEST_NAME} PRIVATE "-lstdc++exp")
endif()


target_include_directories(
  ${TEST_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src
)

# GTest::gmock GTest::gmock_main
include(GoogleTest)
gtest_discover_tests(${TEST_NAME})
