#include <catch2/catch_test_macros.hpp>

#include "raccoon/Lexer.hpp"

TEST_CASE("Lexer tokenizes keywords", "[lexer]") {
  Lexer lexer("fun return let", "test.rac");
  auto tokens = lexer.lex();

  REQUIRE_FALSE(lexer.has_errors());
  REQUIRE(tokens.size() == 4); // FUN, RETURN, LET, EOF
  REQUIRE(tokens[0].type == TokenType::FUN);
  REQUIRE(tokens[1].type == TokenType::RETURN);
  REQUIRE(tokens[2].type == TokenType::LET);
  REQUIRE(tokens[3].type == TokenType::END_OF_FILE);
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

TEST_CASE("Lexer tokenizes operators", "[lexer]") {
  Lexer lexer("+ - * / =", "test.rac");
  auto tokens = lexer.lex();

  REQUIRE_FALSE(lexer.has_errors());
  REQUIRE(tokens.size() == 6); // 5 operators + EOF
  REQUIRE(tokens[0].type == TokenType::PLUS);
  REQUIRE(tokens[0].lexeme == "+");
  REQUIRE(tokens[1].type == TokenType::MINUS);
  REQUIRE(tokens[1].lexeme == "-");
  REQUIRE(tokens[2].type == TokenType::STAR);
  REQUIRE(tokens[2].lexeme == "*");
  REQUIRE(tokens[3].type == TokenType::SLASH);
  REQUIRE(tokens[3].lexeme == "/");
  REQUIRE(tokens[4].type == TokenType::EQUAL);
  REQUIRE(tokens[4].lexeme == "=");
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
  std::string source = "fun // this is a comment\n"
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
  std::string source = "fun main(): i32 {\n"
                       "    return 42;\n"
                       "}";

  Lexer lexer(source, "test.rac");
  auto tokens = lexer.lex();

  REQUIRE_FALSE(lexer.has_errors());
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

TEST_CASE("Lexer tokenizes variable declaration", "[lexer]") {
  std::string source = "let x: i32 = 42;";
  Lexer lexer(source, "test.rac");
  auto tokens = lexer.lex();

  REQUIRE_FALSE(lexer.has_errors());
  // LET x : i32 = 42 ; EOF = 8
  REQUIRE(tokens.size() == 8);
  REQUIRE(tokens[0].type == TokenType::LET);
  REQUIRE(tokens[1].type == TokenType::IDENTIFIER);
  REQUIRE(tokens[1].lexeme == "x");
  REQUIRE(tokens[2].type == TokenType::COLON);
  REQUIRE(tokens[3].type == TokenType::I32);
  REQUIRE(tokens[4].type == TokenType::EQUAL);
  REQUIRE(tokens[5].type == TokenType::INTEGER);
  REQUIRE(tokens[5].int_value == 42);
  REQUIRE(tokens[6].type == TokenType::SEMICOLON);
  REQUIRE(tokens[7].type == TokenType::END_OF_FILE);
}

TEST_CASE("Lexer tokenizes expression with operators", "[lexer]") {
  std::string source = "10 + 20 * 3";
  Lexer lexer(source, "test.rac");
  auto tokens = lexer.lex();

  REQUIRE_FALSE(lexer.has_errors());
  REQUIRE(tokens.size() == 6); // 10 + 20 * 3 EOF
  REQUIRE(tokens[0].type == TokenType::INTEGER);
  REQUIRE(tokens[0].int_value == 10);
  REQUIRE(tokens[1].type == TokenType::PLUS);
  REQUIRE(tokens[2].type == TokenType::INTEGER);
  REQUIRE(tokens[2].int_value == 20);
  REQUIRE(tokens[3].type == TokenType::STAR);
  REQUIRE(tokens[4].type == TokenType::INTEGER);
  REQUIRE(tokens[4].int_value == 3);
}

TEST_CASE("Lexer handles multiple errors", "[lexer]") {
  Lexer lexer("fun @ main # test", "test.rac");
  auto tokens = lexer.lex();

  REQUIRE(lexer.has_errors());
  auto errors = lexer.get_errors();
  REQUIRE(errors.size() == 2); // @ and #
}

TEST_CASE("Lexer distinguishes keywords from identifiers", "[lexer]") {
  Lexer lexer("fun funny return let letter i32 i32x", "test.rac");
  auto tokens = lexer.lex();

  REQUIRE_FALSE(lexer.has_errors());
  REQUIRE(tokens[0].type == TokenType::FUN);
  REQUIRE(tokens[1].type == TokenType::IDENTIFIER); // "funny"
  REQUIRE(tokens[2].type == TokenType::RETURN);
  REQUIRE(tokens[3].type == TokenType::LET);
  REQUIRE(tokens[4].type == TokenType::IDENTIFIER); // "letter"
  REQUIRE(tokens[5].type == TokenType::I32);
  REQUIRE(tokens[6].type == TokenType::IDENTIFIER); // "i32x"
}

TEST_CASE("Lexer handles slash vs comment", "[lexer]") {
  SECTION("single slash is division operator") {
    Lexer lexer("10 / 2", "test.rac");
    auto tokens = lexer.lex();
    REQUIRE_FALSE(lexer.has_errors());
    REQUIRE(tokens.size() == 4);
    REQUIRE(tokens[1].type == TokenType::SLASH);
  }

  SECTION("double slash is comment") {
    Lexer lexer("10 // 2", "test.rac");
    auto tokens = lexer.lex();
    REQUIRE_FALSE(lexer.has_errors());
    REQUIRE(tokens.size() == 2); // 10 EOF (comment swallows rest)
    REQUIRE(tokens[0].type == TokenType::INTEGER);
  }
}
