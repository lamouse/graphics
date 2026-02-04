cmake_minimum_required(VERSION 3.12)
project(ImGuiFileDialog LANGUAGES C CXX)

# Determine if we're built as a subproject (using add_subdirectory)
# or if this is the master project.
set(MASTER_PROJECT OFF)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(MASTER_PROJECT ON)
endif()

# Default to a Release build
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
    message(STATUS "Defaulting to a Release build")
endif()

# Set hard requirements for C++
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
include(${CMAKE_CURRENT_SOURCE_DIR}/source.cmake)

set(LIB_NAME imGuiFileDialog)
add_library(${LIB_NAME} STATIC ${sources})

if(MSVC)
    target_compile_options(${LIB_NAME} PRIVATE
        /MP
        /Zi
        /Zo
        /EHsc
    )

    if(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
        list(APPEND DYNARMIC_CXX_FLAGS
            -Qunused-arguments
            -Wno-missing-braces)
    endif()
else()
    set(DYNARMIC_CXX_FLAGS
        -Wall
        -Wextra
        -Wcast-qual
        -pedantic
        -Wno-missing-braces)

    if(DYNARMIC_WARNINGS_AS_ERRORS)
        list(APPEND DYNARMIC_CXX_FLAGS
            -pedantic-errors
            -Werror)
    endif()

    if(DYNARMIC_FATAL_ERRORS)
        list(APPEND DYNARMIC_CXX_FLAGS
            -Wfatal-errors)
    endif()

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        # GCC produces bogus -Warray-bounds warnings from xbyak headers for code paths that are not
        # actually reachable.  Specifically, it happens in cases where some code casts an Operand&
        # to Address& after first checking isMEM(), and that code is inlined in a situation where
        # GCC knows that the variable is actually a Reg64.  isMEM() will never return true for a
        # Reg64, but GCC doesn't know that.
        list(APPEND DYNARMIC_CXX_FLAGS -Wno-array-bounds)
    endif()

    if(CMAKE_CXX_COMPILER_ID MATCHES "[Cc]lang")
        # Bracket depth determines maximum size of a fold expression in Clang since 9c9974c3ccb6.
        # And this in turns limits the size of a std::array.
        list(APPEND DYNARMIC_CXX_FLAGS -fbracket-depth=1024)
    endif()
endif()

target_link_libraries(${LIB_NAME} PRIVATE Imgui::Imgui)

set(EXPORT_NAME ImGuiFileDialog)
add_library(${EXPORT_NAME} INTERFACE)
add_library(${EXPORT_NAME}::${EXPORT_NAME} ALIAS ${EXPORT_NAME})
target_include_directories(${EXPORT_NAME} INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
target_link_libraries(${EXPORT_NAME} INTERFACE ${LIB_NAME})
