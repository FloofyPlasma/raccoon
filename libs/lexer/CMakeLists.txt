add_library(raccoon_lexer STATIC
  src/Lexer.cpp
)

target_include_directories(raccoon_lexer
  PUBLIC
    ${CMAKE_SOURCE_DIR}/include
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(raccoon_lexer PUBLIC cxx_std_23)
