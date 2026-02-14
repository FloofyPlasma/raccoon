#include <catch2/catch_test_macros.hpp>

#include "raccoon/Lexer.hpp"

TEST_CASE("Lexer tokenizes simple integer", "[lexer]") {
  Lexer lexer("42", "test.rac");
  auto tokens = lexer.lex();
  REQUIRE(tokens.size() == 2);
  REQUIRE(tokens[0].type == TokenType::INTEGER);
  REQUIRE(tokens[0].int_value.value() == 42);
  REQUIRE(tokens[1].type == TokenType::END_OF_FILE);
}

TEST_CASE("Lexer tokenizes keywords", "[lexer]") {
  Lexer lexer("fun return let if else while for break continue", "test.rac");
  auto tokens = lexer.lex();
  REQUIRE(tokens[0].type == TokenType::FUN);
  REQUIRE(tokens[1].type == TokenType::RETURN);
  REQUIRE(tokens[2].type == TokenType::LET);
  REQUIRE(tokens[3].type == TokenType::IF);
  REQUIRE(tokens[4].type == TokenType::ELSE);
  REQUIRE(tokens[5].type == TokenType::WHILE);
  REQUIRE(tokens[6].type == TokenType::FOR);
  REQUIRE(tokens[7].type == TokenType::BREAK);
  REQUIRE(tokens[8].type == TokenType::CONTINUE);
}

TEST_CASE("Lexer tokenizes bool keywords", "[lexer]") {
  Lexer lexer("true false bool", "test.rac");
  auto tokens = lexer.lex();
  REQUIRE(tokens[0].type == TokenType::TRUE_KW);
  REQUIRE(tokens[1].type == TokenType::FALSE_KW);
  REQUIRE(tokens[2].type == TokenType::BOOL);
}

TEST_CASE("Lexer tokenizes arithmetic operators", "[lexer]") {
  Lexer lexer("+ - * /", "test.rac");
  auto tokens = lexer.lex();
  REQUIRE(tokens[0].type == TokenType::PLUS);
  REQUIRE(tokens[1].type == TokenType::MINUS);
  REQUIRE(tokens[2].type == TokenType::STAR);
  REQUIRE(tokens[3].type == TokenType::SLASH);
}

TEST_CASE("Lexer tokenizes comparison operators", "[lexer]") {
  Lexer lexer("< > <= >= == !=", "test.rac");
  auto tokens = lexer.lex();
  REQUIRE(tokens[0].type == TokenType::LESS);
  REQUIRE(tokens[1].type == TokenType::GREATER);
  REQUIRE(tokens[2].type == TokenType::LESS_EQUAL);
  REQUIRE(tokens[3].type == TokenType::GREATER_EQUAL);
  REQUIRE(tokens[4].type == TokenType::EQUAL_EQUAL);
  REQUIRE(tokens[5].type == TokenType::BANG_EQUAL);
}

TEST_CASE("Lexer tokenizes logical operators", "[lexer]") {
  Lexer lexer("&& || !", "test.rac");
  auto tokens = lexer.lex();
  REQUIRE(tokens[0].type == TokenType::AND_AND);
  REQUIRE(tokens[1].type == TokenType::OR_OR);
  REQUIRE(tokens[2].type == TokenType::BANG);
}

TEST_CASE("Lexer distinguishes = from ==", "[lexer]") {
  Lexer lexer("= ==", "test.rac");
  auto tokens = lexer.lex();
  REQUIRE(tokens[0].type == TokenType::EQUAL);
  REQUIRE(tokens[1].type == TokenType::EQUAL_EQUAL);
}

TEST_CASE("Lexer distinguishes < from <=", "[lexer]") {
  Lexer lexer("< <=", "test.rac");
  auto tokens = lexer.lex();
  REQUIRE(tokens[0].type == TokenType::LESS);
  REQUIRE(tokens[1].type == TokenType::LESS_EQUAL);
}

TEST_CASE("Lexer distinguishes ! from !=", "[lexer]") {
  Lexer lexer("! !=", "test.rac");
  auto tokens = lexer.lex();
  REQUIRE(tokens[0].type == TokenType::BANG);
  REQUIRE(tokens[1].type == TokenType::BANG_EQUAL);
}

TEST_CASE("Lexer errors on lone & or |", "[lexer]") {
  SECTION("lone &") {
    Lexer lexer("&", "test.rac");
    auto tokens = lexer.lex();
    REQUIRE(lexer.has_errors());
  }
  SECTION("lone |") {
    Lexer lexer("|", "test.rac");
    auto tokens = lexer.lex();
    REQUIRE(lexer.has_errors());
  }
}

TEST_CASE("Lexer skips comments", "[lexer]") {
  Lexer lexer("42 // this is a comment\n10", "test.rac");
  auto tokens = lexer.lex();
  REQUIRE(tokens.size() == 3);
  REQUIRE(tokens[0].type == TokenType::INTEGER);
  REQUIRE(tokens[1].type == TokenType::INTEGER);
}

TEST_CASE("Lexer handles slash vs comment", "[lexer]") {
  Lexer lexer("10 / 2", "test.rac");
  auto tokens = lexer.lex();
  REQUIRE(tokens[1].type == TokenType::SLASH);
}

TEST_CASE("Lexer tokenizes complex expression", "[lexer]") {
  Lexer lexer("x <= 10 && y > 0", "test.rac");
  auto tokens = lexer.lex();
  REQUIRE(tokens[0].type == TokenType::IDENTIFIER);
  REQUIRE(tokens[1].type == TokenType::LESS_EQUAL);
  REQUIRE(tokens[2].type == TokenType::INTEGER);
  REQUIRE(tokens[3].type == TokenType::AND_AND);
  REQUIRE(tokens[4].type == TokenType::IDENTIFIER);
  REQUIRE(tokens[5].type == TokenType::GREATER);
  REQUIRE(tokens[6].type == TokenType::INTEGER);
}
