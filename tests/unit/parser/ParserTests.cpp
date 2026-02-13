#include <catch2/catch_test_macros.hpp>

#include "raccoon/Parser.hpp"
#include "raccoon/Token.hpp"

std::vector<Token> wrap_in_function(std::vector<Token> body_tokens) {
  std::vector<Token> tokens = {
      {TokenType::FUN, "fun", {"t.rac", 1, 1}},
      {TokenType::IDENTIFIER, "test", {"t.rac", 1, 5}},
      {TokenType::LEFT_PAREN, "(", {"t.rac", 1, 9}},
      {TokenType::RIGHT_PAREN, ")", {"t.rac", 1, 10}},
      {TokenType::COLON, ":", {"t.rac", 1, 11}},
      {TokenType::I32, "i32", {"t.rac", 1, 13}},
      {TokenType::LEFT_BRACE, "{", {"t.rac", 1, 17}},
  };
  for (auto &t : body_tokens) {
    tokens.push_back(t);
  }
  tokens.push_back({TokenType::RIGHT_BRACE, "}", {"t.rac", 99, 1}});
  tokens.push_back({TokenType::END_OF_FILE, "", {"t.rac", 99, 2}});
  return tokens;
}

TEST_CASE("Parser parses return with integer literal", "[parser]") {
  auto tokens = wrap_in_function({
      {TokenType::RETURN, "return", {"t.rac", 2, 5}},
      {TokenType::INTEGER, "42", {"t.rac", 2, 12}, 42},
      {TokenType::SEMICOLON, ";", {"t.rac", 2, 14}},
  });

  Parser parser(std::move(tokens));
  auto result = parser.parse();

  REQUIRE(result.has_value());
  auto &func = result.value()->functions[0];
  REQUIRE(func->body->statements.size() == 1);
  REQUIRE(func->body->statements[0]->kind == Statement::Kind::RETURN);

  auto *ret = static_cast<ReturnStmt *>(func->body->statements[0].get());
  REQUIRE(ret->value->kind == Expression::Kind::INTEGER_LITERAL);
  auto *lit = static_cast<IntegerLiteral *>(ret->value.get());
  REQUIRE(lit->value == 42);
}

TEST_CASE("Parser parses binary expression", "[parser]") {
  auto tokens = wrap_in_function({
      {TokenType::RETURN, "return", {"t.rac", 2, 5}},
      {TokenType::INTEGER, "10", {"t.rac", 2, 12}, 10},
      {TokenType::PLUS, "+", {"t.rac", 2, 15}},
      {TokenType::INTEGER, "20", {"t.rac", 2, 17}, 20},
      {TokenType::SEMICOLON, ";", {"t.rac", 2, 19}},
  });

  Parser parser(std::move(tokens));
  auto result = parser.parse();

  REQUIRE(result.has_value());
  auto *ret = static_cast<ReturnStmt *>(
      result.value()->functions[0]->body->statements[0].get());
  REQUIRE(ret->value->kind == Expression::Kind::BINARY_EXPR);

  auto *bin = static_cast<BinaryExpr *>(ret->value.get());
  REQUIRE(bin->op == TokenType::PLUS);
  REQUIRE(bin->left->kind == Expression::Kind::INTEGER_LITERAL);
  REQUIRE(bin->right->kind == Expression::Kind::INTEGER_LITERAL);
  REQUIRE(static_cast<IntegerLiteral *>(bin->left.get())->value == 10);
  REQUIRE(static_cast<IntegerLiteral *>(bin->right.get())->value == 20);
}

TEST_CASE("Parser respects operator precedence", "[parser]") {
  // 1 + 2 * 3  should parse as  1 + (2 * 3)
  auto tokens = wrap_in_function({
      {TokenType::RETURN, "return", {"t.rac", 2, 5}},
      {TokenType::INTEGER, "1", {"t.rac", 2, 12}, 1},
      {TokenType::PLUS, "+", {"t.rac", 2, 14}},
      {TokenType::INTEGER, "2", {"t.rac", 2, 16}, 2},
      {TokenType::STAR, "*", {"t.rac", 2, 18}},
      {TokenType::INTEGER, "3", {"t.rac", 2, 20}, 3},
      {TokenType::SEMICOLON, ";", {"t.rac", 2, 21}},
  });

  Parser parser(std::move(tokens));
  auto result = parser.parse();
  REQUIRE(result.has_value());

  auto *ret = static_cast<ReturnStmt *>(
      result.value()->functions[0]->body->statements[0].get());
  auto *plus = static_cast<BinaryExpr *>(ret->value.get());

  REQUIRE(plus->op == TokenType::PLUS);
  REQUIRE(plus->left->kind == Expression::Kind::INTEGER_LITERAL);
  REQUIRE(plus->right->kind == Expression::Kind::BINARY_EXPR);

  auto *mul = static_cast<BinaryExpr *>(plus->right.get());
  REQUIRE(mul->op == TokenType::STAR);
}

TEST_CASE("Parser handles left associativity", "[parser]") {
  // 1 - 2 - 3  should parse as  (1 - 2) - 3
  auto tokens = wrap_in_function({
      {TokenType::RETURN, "return", {"t.rac", 2, 5}},
      {TokenType::INTEGER, "1", {"t.rac", 2, 12}, 1},
      {TokenType::MINUS, "-", {"t.rac", 2, 14}},
      {TokenType::INTEGER, "2", {"t.rac", 2, 16}, 2},
      {TokenType::MINUS, "-", {"t.rac", 2, 18}},
      {TokenType::INTEGER, "3", {"t.rac", 2, 20}, 3},
      {TokenType::SEMICOLON, ";", {"t.rac", 2, 21}},
  });

  Parser parser(std::move(tokens));
  auto result = parser.parse();
  REQUIRE(result.has_value());

  auto *ret = static_cast<ReturnStmt *>(
      result.value()->functions[0]->body->statements[0].get());
  auto *outer = static_cast<BinaryExpr *>(ret->value.get());

  // Outer is (1 - 2) - 3
  REQUIRE(outer->op == TokenType::MINUS);
  REQUIRE(outer->left->kind == Expression::Kind::BINARY_EXPR);
  REQUIRE(outer->right->kind == Expression::Kind::INTEGER_LITERAL);
  REQUIRE(static_cast<IntegerLiteral *>(outer->right.get())->value == 3);

  auto *inner = static_cast<BinaryExpr *>(outer->left.get());
  REQUIRE(inner->op == TokenType::MINUS);
  REQUIRE(static_cast<IntegerLiteral *>(inner->left.get())->value == 1);
  REQUIRE(static_cast<IntegerLiteral *>(inner->right.get())->value == 2);
}

TEST_CASE("Parser parses parenthesized expressions", "[parser]") {
  // (1 + 2) * 3  should parse as  (1 + 2) * 3
  auto tokens = wrap_in_function({
      {TokenType::RETURN, "return", {"t.rac", 2, 5}},
      {TokenType::LEFT_PAREN, "(", {"t.rac", 2, 12}},
      {TokenType::INTEGER, "1", {"t.rac", 2, 13}, 1},
      {TokenType::PLUS, "+", {"t.rac", 2, 15}},
      {TokenType::INTEGER, "2", {"t.rac", 2, 17}, 2},
      {TokenType::RIGHT_PAREN, ")", {"t.rac", 2, 18}},
      {TokenType::STAR, "*", {"t.rac", 2, 20}},
      {TokenType::INTEGER, "3", {"t.rac", 2, 22}, 3},
      {TokenType::SEMICOLON, ";", {"t.rac", 2, 23}},
  });

  Parser parser(std::move(tokens));
  auto result = parser.parse();
  REQUIRE(result.has_value());

  auto *ret = static_cast<ReturnStmt *>(
      result.value()->functions[0]->body->statements[0].get());
  auto *mul = static_cast<BinaryExpr *>(ret->value.get());

  REQUIRE(mul->op == TokenType::STAR);
  REQUIRE(mul->left->kind == Expression::Kind::BINARY_EXPR);

  auto *add = static_cast<BinaryExpr *>(mul->left.get());
  REQUIRE(add->op == TokenType::PLUS);
}

TEST_CASE("Parser parses variable declaration with initializer", "[parser]") {
  auto tokens = wrap_in_function({
      {TokenType::LET, "let", {"t.rac", 2, 5}},
      {TokenType::IDENTIFIER, "x", {"t.rac", 2, 9}},
      {TokenType::COLON, ":", {"t.rac", 2, 10}},
      {TokenType::I32, "i32", {"t.rac", 2, 12}},
      {TokenType::EQUAL, "=", {"t.rac", 2, 16}},
      {TokenType::INTEGER, "42", {"t.rac", 2, 18}, 42},
      {TokenType::SEMICOLON, ";", {"t.rac", 2, 20}},
      {TokenType::RETURN, "return", {"t.rac", 3, 5}},
      {TokenType::INTEGER, "0", {"t.rac", 3, 12}, 0},
      {TokenType::SEMICOLON, ";", {"t.rac", 3, 13}},
  });

  Parser parser(std::move(tokens));
  auto result = parser.parse();
  REQUIRE(result.has_value());

  auto &stmts = result.value()->functions[0]->body->statements;
  REQUIRE(stmts.size() == 2);
  REQUIRE(stmts[0]->kind == Statement::Kind::VAR_DECL);

  auto *var = static_cast<VarDeclStmt *>(stmts[0].get());
  REQUIRE(var->name == "x");
  REQUIRE(var->type->kind == Type::Kind::I32);
  REQUIRE(var->initializer != nullptr);
  REQUIRE(var->initializer->kind == Expression::Kind::INTEGER_LITERAL);
  REQUIRE(static_cast<IntegerLiteral *>(var->initializer.get())->value == 42);
}

TEST_CASE("Parser parses uninitialized variable declaration", "[parser]") {
  auto tokens = wrap_in_function({
      {TokenType::LET, "let", {"t.rac", 2, 5}},
      {TokenType::IDENTIFIER, "x", {"t.rac", 2, 9}},
      {TokenType::COLON, ":", {"t.rac", 2, 10}},
      {TokenType::I32, "i32", {"t.rac", 2, 12}},
      {TokenType::SEMICOLON, ";", {"t.rac", 2, 15}},
      {TokenType::RETURN, "return", {"t.rac", 3, 5}},
      {TokenType::INTEGER, "0", {"t.rac", 3, 12}, 0},
      {TokenType::SEMICOLON, ";", {"t.rac", 3, 13}},
  });

  Parser parser(std::move(tokens));
  auto result = parser.parse();
  REQUIRE(result.has_value());

  auto *var = static_cast<VarDeclStmt *>(
      result.value()->functions[0]->body->statements[0].get());
  REQUIRE(var->name == "x");
  REQUIRE(var->initializer == nullptr);
}

TEST_CASE("Parser parses assignment statement", "[parser]") {
  auto tokens = wrap_in_function({
      {TokenType::LET, "let", {"t.rac", 2, 5}},
      {TokenType::IDENTIFIER, "x", {"t.rac", 2, 9}},
      {TokenType::COLON, ":", {"t.rac", 2, 10}},
      {TokenType::I32, "i32", {"t.rac", 2, 12}},
      {TokenType::EQUAL, "=", {"t.rac", 2, 16}},
      {TokenType::INTEGER, "5", {"t.rac", 2, 18}, 5},
      {TokenType::SEMICOLON, ";", {"t.rac", 2, 19}},
      // x = 10;
      {TokenType::IDENTIFIER, "x", {"t.rac", 3, 5}},
      {TokenType::EQUAL, "=", {"t.rac", 3, 7}},
      {TokenType::INTEGER, "10", {"t.rac", 3, 9}, 10},
      {TokenType::SEMICOLON, ";", {"t.rac", 3, 11}},
      {TokenType::RETURN, "return", {"t.rac", 4, 5}},
      {TokenType::INTEGER, "0", {"t.rac", 4, 12}, 0},
      {TokenType::SEMICOLON, ";", {"t.rac", 4, 13}},
  });

  Parser parser(std::move(tokens));
  auto result = parser.parse();
  REQUIRE(result.has_value());

  auto &stmts = result.value()->functions[0]->body->statements;
  REQUIRE(stmts.size() == 3);
  REQUIRE(stmts[1]->kind == Statement::Kind::ASSIGNMENT);

  auto *assign = static_cast<AssignmentStmt *>(stmts[1].get());
  REQUIRE(assign->name == "x");
  REQUIRE(assign->value->kind == Expression::Kind::INTEGER_LITERAL);
  REQUIRE(static_cast<IntegerLiteral *>(assign->value.get())->value == 10);
}

TEST_CASE("Parser parses expression statement", "[parser]") {
  auto tokens = wrap_in_function({
      {TokenType::INTEGER, "42", {"t.rac", 2, 5}, 42},
      {TokenType::PLUS, "+", {"t.rac", 2, 8}},
      {TokenType::INTEGER, "1", {"t.rac", 2, 10}, 1},
      {TokenType::SEMICOLON, ";", {"t.rac", 2, 11}},
      {TokenType::RETURN, "return", {"t.rac", 3, 5}},
      {TokenType::INTEGER, "0", {"t.rac", 3, 12}, 0},
      {TokenType::SEMICOLON, ";", {"t.rac", 3, 13}},
  });

  Parser parser(std::move(tokens));
  auto result = parser.parse();
  REQUIRE(result.has_value());

  auto &stmts = result.value()->functions[0]->body->statements;
  REQUIRE(stmts.size() == 2);
  REQUIRE(stmts[0]->kind == Statement::Kind::EXPRESSION);
}

TEST_CASE("Parser parses identifier in expression", "[parser]") {
  auto tokens = wrap_in_function({
      {TokenType::RETURN, "return", {"t.rac", 2, 5}},
      {TokenType::IDENTIFIER, "x", {"t.rac", 2, 12}},
      {TokenType::SEMICOLON, ";", {"t.rac", 2, 13}},
  });

  Parser parser(std::move(tokens));
  auto result = parser.parse();
  REQUIRE(result.has_value());

  auto *ret = static_cast<ReturnStmt *>(
      result.value()->functions[0]->body->statements[0].get());
  REQUIRE(ret->value->kind == Expression::Kind::IDENTIFIER);
  REQUIRE(static_cast<IdentifierExpr *>(ret->value.get())->name == "x");
}

TEST_CASE("Parser reports missing semicolon", "[parser]") {
  auto tokens = wrap_in_function({
      {TokenType::RETURN, "return", {"t.rac", 2, 5}},
      {TokenType::INTEGER, "42", {"t.rac", 2, 12}, 42},
      // Missing SEMICOLON
  });

  Parser parser(std::move(tokens));
  auto result = parser.parse();

  REQUIRE_FALSE(result.has_value());
  auto &errors = result.error();
  REQUIRE(errors.size() >= 1);
  REQUIRE(errors[0].message.find("';'") != std::string::npos);
}

TEST_CASE("Parser parses assignment with expression RHS", "[parser]") {
  // x = x + 10;
  auto tokens = wrap_in_function({
      {TokenType::LET, "let", {"t.rac", 2, 5}},
      {TokenType::IDENTIFIER, "x", {"t.rac", 2, 9}},
      {TokenType::COLON, ":", {"t.rac", 2, 10}},
      {TokenType::I32, "i32", {"t.rac", 2, 12}},
      {TokenType::EQUAL, "=", {"t.rac", 2, 16}},
      {TokenType::INTEGER, "5", {"t.rac", 2, 18}, 5},
      {TokenType::SEMICOLON, ";", {"t.rac", 2, 19}},
      {TokenType::IDENTIFIER, "x", {"t.rac", 3, 5}},
      {TokenType::EQUAL, "=", {"t.rac", 3, 7}},
      {TokenType::IDENTIFIER, "x", {"t.rac", 3, 9}},
      {TokenType::PLUS, "+", {"t.rac", 3, 11}},
      {TokenType::INTEGER, "10", {"t.rac", 3, 13}, 10},
      {TokenType::SEMICOLON, ";", {"t.rac", 3, 15}},
      {TokenType::RETURN, "return", {"t.rac", 4, 5}},
      {TokenType::IDENTIFIER, "x", {"t.rac", 4, 12}},
      {TokenType::SEMICOLON, ";", {"t.rac", 4, 13}},
  });

  Parser parser(std::move(tokens));
  auto result = parser.parse();
  REQUIRE(result.has_value());

  auto &stmts = result.value()->functions[0]->body->statements;
  REQUIRE(stmts.size() == 3);

  auto *assign = static_cast<AssignmentStmt *>(stmts[1].get());
  REQUIRE(assign->name == "x");
  REQUIRE(assign->value->kind == Expression::Kind::BINARY_EXPR);
}

TEST_CASE("Parser parses all arithmetic operators", "[parser]") {
  // Tests each: + - * /
  auto test_op = [](TokenType op_type) {
    auto tokens = wrap_in_function({
        {TokenType::RETURN, "return", {"t.rac", 2, 5}},
        {TokenType::INTEGER, "1", {"t.rac", 2, 12}, 1},
        {op_type, "op", {"t.rac", 2, 14}},
        {TokenType::INTEGER, "2", {"t.rac", 2, 16}, 2},
        {TokenType::SEMICOLON, ";", {"t.rac", 2, 17}},
    });

    Parser parser(std::move(tokens));
    auto result = parser.parse();
    REQUIRE(result.has_value());

    auto *ret = static_cast<ReturnStmt *>(
        result.value()->functions[0]->body->statements[0].get());
    REQUIRE(ret->value->kind == Expression::Kind::BINARY_EXPR);
    auto *bin = static_cast<BinaryExpr *>(ret->value.get());
    REQUIRE(bin->op == op_type);
  };

  SECTION("addition") { test_op(TokenType::PLUS); }
  SECTION("subtraction") { test_op(TokenType::MINUS); }
  SECTION("multiplication") { test_op(TokenType::STAR); }
  SECTION("division") { test_op(TokenType::SLASH); }
}
