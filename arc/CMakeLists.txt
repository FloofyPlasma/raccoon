cmake_minimum_required(VERSION 3.30)
project(arc VERSION 0.1.0 LANGUAGES C CXX)

option(ARC_BUILD_TESTS "Build Arc tests" OFF)
option(ARC_BUILD_LLVM "Build LLVM backend" OFF)

add_library(arc_arc STATIC
        src/ir.cpp
)

add_library(arc::arc ALIAS arc_arc)

target_include_directories(arc_arc
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(arc_arc PUBLIC cxx_std_23)

target_compile_options(arc_arc PRIVATE
        $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -Wpedantic>
        $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -Wpedantic>
)

if (ARC_BUILD_LLVM)
    find_package(LLVM 22.1 REQUIRED CONFIG)

    message(STATUS "Arc: Found LLVM ${LLVM_PACKAGE_VERSION}")

    add_library(arc_llvm STATIC
    )

    add_library(arc::llvm ALIAS arc_llvm)

    target_include_directories(arc_llvm
            PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    )

    target_link_libraries(arc_llvm
            PUBLIC arc::arc
            PRIVATE LLVM
    )
endif ()

if (ARC_BUILD_TESTS)
    find_package(Catch2 3 REQUIRED)
    add_subdirectory(tests)
endif ()
