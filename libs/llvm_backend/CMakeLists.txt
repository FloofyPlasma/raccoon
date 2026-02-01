add_library(raccoon_llvm_backend STATIC
  src/LLVMBackend.cpp
)

target_include_directories(raccoon_llvm_backend
  PUBLIC
    ${CMAKE_SOURCE_DIR}/include
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(raccoon_llvm_backend
  PUBLIC
    raccoon_ir
)

target_link_libraries(raccoon_llvm_backend
  PRIVATE
    LLVM
)

target_compile_features(raccoon_llvm_backend PUBLIC cxx_std_23)
