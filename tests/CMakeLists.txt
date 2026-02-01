include(FetchContent)
FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.12.0
)
FetchContent_MakeAvailable(Catch2)

add_executable(unit_tests
        unit/test_main.cpp
        unit/lexer/LexerTests.cpp
        unit/parser/ParserTests.cpp
)

target_link_libraries(unit_tests
        PRIVATE
        raccoon_lexer
        raccoon_parser
        Catch2::Catch2WithMain
)

target_compile_features(unit_tests PUBLIC cxx_std_23)

include(CTest)
include(Catch)
catch_discover_tests(unit_tests)
