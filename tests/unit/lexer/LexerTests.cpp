#include "raccoon/Lexer.hpp"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Lexer tokenizes keywords", "[lexer]") {
    Lexer lexer("fun return", "test.rac");
    auto tokens = lexer.lex();

    REQUIRE_FALSE(lexer.has_errors());
    REQUIRE(tokens.size() == 3); // FUN, RETURN, EOF
    REQUIRE(tokens[0].type == TokenType::FUN);
    REQUIRE(tokens[0].lexeme == "fun");
    REQUIRE(tokens[1].type == TokenType::RETURN);
    REQUIRE(tokens[1].lexeme == "return");
    REQUIRE(tokens[2].type == TokenType::END_OF_FILE);
}

TEST_CASE("Lexer tokenizes type keywords", "[lexer]") {
    Lexer lexer("i32", "test.rac");
    auto tokens = lexer.lex();

    REQUIRE_FALSE(lexer.has_errors());
    REQUIRE(tokens.size() == 2); // I32, EOF
    REQUIRE(tokens[0].type == TokenType::I32);
    REQUIRE(tokens[0].lexeme == "i32");
}

TEST_CASE("Lexer tokenizes identifiers", "[lexer]") {
    Lexer lexer("main myFunc _private", "test.rac");
    auto tokens = lexer.lex();

    REQUIRE_FALSE(lexer.has_errors());
    REQUIRE(tokens.size() == 4); // 3 identifiers + EOF
    REQUIRE(tokens[0].type == TokenType::IDENTIFIER);
    REQUIRE(tokens[0].lexeme == "main");
    REQUIRE(tokens[1].type == TokenType::IDENTIFIER);
    REQUIRE(tokens[1].lexeme == "myFunc");
    REQUIRE(tokens[2].type == TokenType::IDENTIFIER);
    REQUIRE(tokens[2].lexeme == "_private");
}

TEST_CASE("Lexer tokenizes integers", "[lexer]") {
    Lexer lexer("0 42 123 999", "test.rac");
    auto tokens = lexer.lex();

    REQUIRE_FALSE(lexer.has_errors());
    REQUIRE(tokens.size() == 5); // 4 integers + EOF
    REQUIRE(tokens[0].type == TokenType::INTEGER);
    REQUIRE(tokens[0].int_value == 0);
    REQUIRE(tokens[1].type == TokenType::INTEGER);
    REQUIRE(tokens[1].int_value == 42);
    REQUIRE(tokens[2].int_value == 123);
    REQUIRE(tokens[3].int_value == 999);
}

TEST_CASE("Lexer tokenizes punctuation", "[lexer]") {
    Lexer lexer("():;{}", "test.rac");
    auto tokens = lexer.lex();

    REQUIRE_FALSE(lexer.has_errors());
    REQUIRE(tokens.size() == 7); // 6 punctuation + EOF
    REQUIRE(tokens[0].type == TokenType::LEFT_PAREN);
    REQUIRE(tokens[1].type == TokenType::RIGHT_PAREN);
    REQUIRE(tokens[2].type == TokenType::COLON);
    REQUIRE(tokens[3].type == TokenType::SEMICOLON);
    REQUIRE(tokens[4].type == TokenType::LEFT_BRACE);
    REQUIRE(tokens[5].type == TokenType::RIGHT_BRACE);
}

TEST_CASE("Lexer handles whitespace", "[lexer]") {
    Lexer lexer("  fun   main  \n\t return  ", "test.rac");
    auto tokens = lexer.lex();

    REQUIRE_FALSE(lexer.has_errors());
    REQUIRE(tokens.size() == 4); // FUN, IDENTIFIER, RETURN, EOF
    REQUIRE(tokens[0].type == TokenType::FUN);
    REQUIRE(tokens[1].type == TokenType::IDENTIFIER);
    REQUIRE(tokens[2].type == TokenType::RETURN);
}

TEST_CASE("Lexer handles line comments", "[lexer]") {
    std::string source =
        "fun // this is a comment\n"
        "main // another comment";

    Lexer lexer(source, "test.rac");
    auto tokens = lexer.lex();

    REQUIRE_FALSE(lexer.has_errors());
    REQUIRE(tokens.size() == 3); // FUN, IDENTIFIER, EOF
    REQUIRE(tokens[0].type == TokenType::FUN);
    REQUIRE(tokens[1].type == TokenType::IDENTIFIER);
}

TEST_CASE("Lexer tracks location correctly", "[lexer]") {
    std::string source = "fun\nmain";
    Lexer lexer(source, "test.rac");
    auto tokens = lexer.lex();

    REQUIRE_FALSE(lexer.has_errors());
    REQUIRE(tokens[0].location.line == 1);
    REQUIRE(tokens[0].location.column == 1);
    REQUIRE(tokens[0].location.filename == "test.rac");

    REQUIRE(tokens[1].location.line == 2);
    REQUIRE(tokens[1].location.column == 1);
}

TEST_CASE("Lexer reports errors for unexpected characters", "[lexer]") {
    Lexer lexer("fun @ main", "test.rac");
    auto tokens = lexer.lex();

    REQUIRE(lexer.has_errors());
    auto errors = lexer.get_errors();
    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0].message.find("Unexpected character") != std::string::npos);
    REQUIRE(errors[0].location.line == 1);
}

TEST_CASE("Lexer tokenizes complete minimal program", "[lexer]") {
    std::string source =
        "fun main(): i32 {\n"
        "    return 42;\n"
        "}";

    Lexer lexer(source, "test.rac");
    auto tokens = lexer.lex();

    REQUIRE_FALSE(lexer.has_errors());
    // Expected: FUN IDENTIFIER LPAREN RPAREN COLON I32 LBRACE
    //           RETURN INTEGER SEMICOLON RBRACE EOF
    REQUIRE(tokens.size() == 12);

    REQUIRE(tokens[0].type == TokenType::FUN);
    REQUIRE(tokens[1].type == TokenType::IDENTIFIER);
    REQUIRE(tokens[1].lexeme == "main");
    REQUIRE(tokens[2].type == TokenType::LEFT_PAREN);
    REQUIRE(tokens[3].type == TokenType::RIGHT_PAREN);
    REQUIRE(tokens[4].type == TokenType::COLON);
    REQUIRE(tokens[5].type == TokenType::I32);
    REQUIRE(tokens[6].type == TokenType::LEFT_BRACE);
    REQUIRE(tokens[7].type == TokenType::RETURN);
    REQUIRE(tokens[8].type == TokenType::INTEGER);
    REQUIRE(tokens[8].int_value == 42);
    REQUIRE(tokens[9].type == TokenType::SEMICOLON);
    REQUIRE(tokens[10].type == TokenType::RIGHT_BRACE);
    REQUIRE(tokens[11].type == TokenType::END_OF_FILE);
}

TEST_CASE("Lexer handles multiple errors", "[lexer]") {
    Lexer lexer("fun @ main # test", "test.rac");
    auto tokens = lexer.lex();

    REQUIRE(lexer.has_errors());
    auto errors = lexer.get_errors();
    REQUIRE(errors.size() == 2); // @ and #
}

TEST_CASE("Lexer distinguishes keywords from identifiers", "[lexer]") {
    Lexer lexer("fun funny return i32 i32x", "test.rac");
    auto tokens = lexer.lex();

    REQUIRE_FALSE(lexer.has_errors());
    REQUIRE(tokens[0].type == TokenType::FUN);
    REQUIRE(tokens[1].type == TokenType::IDENTIFIER); // "funny"
    REQUIRE(tokens[2].type == TokenType::RETURN);
    REQUIRE(tokens[3].type == TokenType::I32);
    REQUIRE(tokens[4].type == TokenType::IDENTIFIER); // "i32x"
}