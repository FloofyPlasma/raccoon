add_library(raccoon_parser STATIC
        src/Parser.cpp
        src/AST.cpp
)

target_include_directories(raccoon_parser
        PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(raccoon_parser
        PUBLIC
        raccoon_lexer
)

target_compile_features(raccoon_parser PUBLIC cxx_std_23)
