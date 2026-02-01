add_executable(raccoonc
        main.cpp
)

target_link_libraries(raccoonc
        raccoon_lexer
        raccoon_parser
        raccoon_semantic
        raccoon_ir
        raccoon_llvm_backend
        raccoon_linker
)

target_compile_features(raccoonc PUBLIC cxx_std_23)
