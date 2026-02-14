#include <catch2/catch_test_macros.hpp>

#include "raccoon/Lexer.hpp"
#include "raccoon/Parser.hpp"

std::unique_ptr<ProgramNode> parse_source(const std::string &source) {
  Lexer lexer(source, "test.rac");
  auto tokens = lexer.lex();
  if (lexer.has_errors())
    return nullptr;

  Parser parser(std::move(tokens));
  auto result = parser.parse();
  if (!result.has_value())
    return nullptr;
  return std::move(result.value());
}

Statement *first_stmt(ProgramNode *prog) {
  return prog->functions[0]->body->statements[0].get();
}

Expression *return_expr(ProgramNode *prog) {
  auto *ret = static_cast<ReturnStmt *>(first_stmt(prog));
  return ret->value.get();
}

TEST_CASE("Parser parses return with integer literal", "[parser]") {
  auto ast = parse_source("fun main(): i32 { return 42; }");
  REQUIRE(ast != nullptr);
  REQUIRE(first_stmt(ast.get())->kind == Statement::Kind::RETURN);
  auto *lit = static_cast<IntegerLiteral *>(return_expr(ast.get()));
  REQUIRE(lit->value == 42);
}

TEST_CASE("Parser parses binary expression", "[parser]") {
  auto ast = parse_source("fun main(): i32 { return 10 + 20; }");
  REQUIRE(ast != nullptr);
  auto *bin = static_cast<BinaryExpr *>(return_expr(ast.get()));
  REQUIRE(bin->op == TokenType::PLUS);
}

TEST_CASE("Parser respects * over + precedence", "[parser]") {
  auto ast = parse_source("fun main(): i32 { return 1 + 2 * 3; }");
  REQUIRE(ast != nullptr);
  auto *plus = static_cast<BinaryExpr *>(return_expr(ast.get()));
  REQUIRE(plus->op == TokenType::PLUS);
  REQUIRE(plus->right->kind == Expression::Kind::BINARY_EXPR);
  auto *mul = static_cast<BinaryExpr *>(plus->right.get());
  REQUIRE(mul->op == TokenType::STAR);
}

TEST_CASE("Parser handles left associativity", "[parser]") {
  auto ast = parse_source("fun main(): i32 { return 1 - 2 - 3; }");
  REQUIRE(ast != nullptr);
  auto *outer = static_cast<BinaryExpr *>(return_expr(ast.get()));
  REQUIRE(outer->op == TokenType::MINUS);
  REQUIRE(outer->left->kind == Expression::Kind::BINARY_EXPR);
  REQUIRE(outer->right->kind == Expression::Kind::INTEGER_LITERAL);
}

TEST_CASE("Parser parses parenthesized expressions", "[parser]") {
  auto ast = parse_source("fun main(): i32 { return (1 + 2) * 3; }");
  REQUIRE(ast != nullptr);
  auto *mul = static_cast<BinaryExpr *>(return_expr(ast.get()));
  REQUIRE(mul->op == TokenType::STAR);
  REQUIRE(mul->left->kind == Expression::Kind::BINARY_EXPR);
}

TEST_CASE("Parser parses comparison operators", "[parser]") {
  for (auto [src, expected_op] :
       std::initializer_list<std::pair<const char *, TokenType>>{
           {"fun f(): bool { return 1 < 2; }", TokenType::LESS},
           {"fun f(): bool { return 1 > 2; }", TokenType::GREATER},
           {"fun f(): bool { return 1 <= 2; }", TokenType::LESS_EQUAL},
           {"fun f(): bool { return 1 >= 2; }", TokenType::GREATER_EQUAL},
           {"fun f(): bool { return 1 == 2; }", TokenType::EQUAL_EQUAL},
           {"fun f(): bool { return 1 != 2; }", TokenType::BANG_EQUAL},
       }) {
    auto ast = parse_source(src);
    REQUIRE(ast != nullptr);
    auto *bin = static_cast<BinaryExpr *>(return_expr(ast.get()));
    REQUIRE(bin->op == expected_op);
  }
}

TEST_CASE("Parser parses logical operators", "[parser]") {
  auto ast = parse_source("fun f(): bool { return true && false; }");
  REQUIRE(ast != nullptr);
  auto *bin = static_cast<BinaryExpr *>(return_expr(ast.get()));
  REQUIRE(bin->op == TokenType::AND_AND);
}

TEST_CASE("Parser precedence: comparison lower than arithmetic", "[parser]") {
  // 1 + 2 < 3 + 4  →  (1+2) < (3+4)
  auto ast = parse_source("fun f(): bool { return 1 + 2 < 3 + 4; }");
  REQUIRE(ast != nullptr);
  auto *cmp = static_cast<BinaryExpr *>(return_expr(ast.get()));
  REQUIRE(cmp->op == TokenType::LESS);
  REQUIRE(cmp->left->kind == Expression::Kind::BINARY_EXPR);
  REQUIRE(cmp->right->kind == Expression::Kind::BINARY_EXPR);
}

TEST_CASE("Parser precedence: && lower than ==", "[parser]") {
  // a == b && c == d  →  (a==b) && (c==d)
  auto ast =
      parse_source("fun f(): bool { let a: i32 = 1; let b: i32 = 1; let c: i32 "
                   "= 1; let d: i32 = 1; return a == b && c == d; }");
  REQUIRE(ast != nullptr);
  // last stmt is return
  auto &stmts = ast->functions[0]->body->statements;
  auto *ret = static_cast<ReturnStmt *>(stmts.back().get());
  auto *land = static_cast<BinaryExpr *>(ret->value.get());
  REQUIRE(land->op == TokenType::AND_AND);
  REQUIRE(land->left->kind == Expression::Kind::BINARY_EXPR);
  REQUIRE(land->right->kind == Expression::Kind::BINARY_EXPR);
}

TEST_CASE("Parser parses unary minus", "[parser]") {
  auto ast = parse_source("fun main(): i32 { return -42; }");
  REQUIRE(ast != nullptr);
  auto *unary = static_cast<UnaryExpr *>(return_expr(ast.get()));
  REQUIRE(unary->op == TokenType::MINUS);
  REQUIRE(unary->operand->kind == Expression::Kind::INTEGER_LITERAL);
}

TEST_CASE("Parser parses unary not", "[parser]") {
  auto ast = parse_source("fun f(): bool { return !true; }");
  REQUIRE(ast != nullptr);
  auto *unary = static_cast<UnaryExpr *>(return_expr(ast.get()));
  REQUIRE(unary->op == TokenType::BANG);
  REQUIRE(unary->operand->kind == Expression::Kind::BOOL_LITERAL);
}

TEST_CASE("Parser parses bool literals", "[parser]") {
  auto ast = parse_source("fun f(): bool { return true; }");
  REQUIRE(ast != nullptr);
  auto *lit = static_cast<BoolLiteral *>(return_expr(ast.get()));
  REQUIRE(lit->value == true);
}

TEST_CASE("Parser parses if statement", "[parser]") {
  auto ast =
      parse_source("fun main(): i32 { if (true) { return 1; } return 0; }");
  REQUIRE(ast != nullptr);
  REQUIRE(first_stmt(ast.get())->kind == Statement::Kind::IF);
  auto *if_stmt = static_cast<IfStmt *>(first_stmt(ast.get()));
  REQUIRE(if_stmt->condition != nullptr);
  REQUIRE(if_stmt->then_branch != nullptr);
  REQUIRE(if_stmt->else_branch == nullptr);
}

TEST_CASE("Parser parses if/else statement", "[parser]") {
  auto ast = parse_source(
      "fun main(): i32 { if (true) { return 1; } else { return 0; } }");
  REQUIRE(ast != nullptr);
  auto *if_stmt = static_cast<IfStmt *>(first_stmt(ast.get()));
  REQUIRE(if_stmt->else_branch != nullptr);
}

TEST_CASE("Parser parses else-if chain", "[parser]") {
  auto ast = parse_source(R"(
        fun main(): i32 {
            if (true) { return 1; }
            else if (false) { return 2; }
            else { return 3; }
        }
    )");
  REQUIRE(ast != nullptr);
  auto *if_stmt = static_cast<IfStmt *>(first_stmt(ast.get()));
  REQUIRE(if_stmt->else_branch != nullptr);
  // else branch is a synthetic block containing an if stmt
  REQUIRE(if_stmt->else_branch->statements.size() == 1);
  REQUIRE(if_stmt->else_branch->statements[0]->kind == Statement::Kind::IF);
}

TEST_CASE("Parser parses while statement", "[parser]") {
  auto ast =
      parse_source("fun main(): i32 { while (true) { break; } return 0; }");
  REQUIRE(ast != nullptr);
  REQUIRE(first_stmt(ast.get())->kind == Statement::Kind::WHILE);
  auto *while_stmt = static_cast<WhileStmt *>(first_stmt(ast.get()));
  REQUIRE(while_stmt->condition != nullptr);
  REQUIRE(while_stmt->body != nullptr);
}

TEST_CASE("Parser parses for statement", "[parser]") {
  auto ast = parse_source(R"(
        fun main(): i32 {
            for (let i: i32 = 0; i < 10; i = i + 1) {
                i = i;
            }
            return 0;
        }
    )");
  REQUIRE(ast != nullptr);
  REQUIRE(first_stmt(ast.get())->kind == Statement::Kind::FOR);
  auto *for_stmt = static_cast<ForStmt *>(first_stmt(ast.get()));
  REQUIRE(for_stmt->initializer != nullptr);
  REQUIRE(for_stmt->condition != nullptr);
  REQUIRE(for_stmt->increment != nullptr);
  REQUIRE(for_stmt->body != nullptr);
}

TEST_CASE("Parser parses for with empty parts", "[parser]") {
  auto ast = parse_source("fun main(): i32 { for (;;) { break; } return 0; }");
  REQUIRE(ast != nullptr);
  auto *for_stmt = static_cast<ForStmt *>(first_stmt(ast.get()));
  REQUIRE(for_stmt->initializer == nullptr);
  REQUIRE(for_stmt->condition == nullptr);
  REQUIRE(for_stmt->increment == nullptr);
}

TEST_CASE("Parser parses break and continue", "[parser]") {
  auto ast = parse_source(R"(
        fun main(): i32 {
            while (true) {
                break;
                continue;
            }
            return 0;
        }
    )");
  REQUIRE(ast != nullptr);
  auto *while_stmt = static_cast<WhileStmt *>(first_stmt(ast.get()));
  REQUIRE(while_stmt->body->statements[0]->kind == Statement::Kind::BREAK);
  REQUIRE(while_stmt->body->statements[1]->kind == Statement::Kind::CONTINUE);
}

TEST_CASE("Parser parses variable declaration", "[parser]") {
  auto ast = parse_source("fun main(): i32 { let x: i32 = 42; return x; }");
  REQUIRE(ast != nullptr);
  auto *var = static_cast<VarDeclStmt *>(first_stmt(ast.get()));
  REQUIRE(var->name == "x");
  REQUIRE(var->type->kind == Type::Kind::I32);
  REQUIRE(var->initializer != nullptr);
}

TEST_CASE("Parser parses bool variable", "[parser]") {
  auto ast =
      parse_source("fun main(): i32 { let flag: bool = true; return 0; }");
  REQUIRE(ast != nullptr);
  auto *var = static_cast<VarDeclStmt *>(first_stmt(ast.get()));
  REQUIRE(var->name == "flag");
  REQUIRE(var->type->kind == Type::Kind::BOOL);
}

TEST_CASE("Parser parses assignment", "[parser]") {
  auto ast =
      parse_source("fun main(): i32 { let x: i32 = 5; x = 10; return x; }");
  REQUIRE(ast != nullptr);
  auto &stmts = ast->functions[0]->body->statements;
  REQUIRE(stmts[1]->kind == Statement::Kind::ASSIGNMENT);
}
