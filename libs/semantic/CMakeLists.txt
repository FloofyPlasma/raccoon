add_library(raccoon_semantic STATIC
        src/SemanticAnalyzer.cpp
        src/SymbolTable.cpp
)

target_include_directories(raccoon_semantic
        PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(raccoon_semantic
        PUBLIC
        raccoon_parser
)

target_compile_features(raccoon_semantic PUBLIC cxx_std_23)
