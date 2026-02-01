add_library(raccoon_ir STATIC
  src/IRGenerator.cpp
        src/IR.cpp
        src/IRPrinter.cpp
)

target_include_directories(raccoon_ir
  PUBLIC
    ${CMAKE_SOURCE_DIR}/include
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(raccoon_ir
  PUBLIC
    raccoon_semantic
)

target_compile_features(raccoon_ir PUBLIC cxx_std_23)
