cmake_minimum_required(VERSION 3.30)
project(RaccoonCompiler VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

add_compile_options(-Wall -Wextra -Werror -Wpedantic)

find_package(LLVM 21.1.6 REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

find_package(LLD REQUIRED CONFIG)
message(STATUS "Using LLDConfig.cmake in: ${LLD_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

include_directories(${CMAKE_SOURCE_DIR}/include)

add_subdirectory(libs/lexer)
add_subdirectory(libs/parser)
add_subdirectory(libs/semantic)
add_subdirectory(libs/ir)
add_subdirectory(libs/llvm_backend)
add_subdirectory(libs/linker)
add_subdirectory(src)

enable_testing()
add_subdirectory(tests)
