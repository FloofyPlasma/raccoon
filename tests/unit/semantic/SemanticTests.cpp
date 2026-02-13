#include <catch2/catch_test_macros.hpp>

#include "raccoon/Lexer.hpp"
#include "raccoon/Parser.hpp"
#include "raccoon/SemanticAnalyzer.hpp"

struct AnalysisResult {
  std::unique_ptr<ProgramNode> ast;
  std::vector<SemanticError> errors;
  bool success;
};

AnalysisResult analyze_source(const std::string &source) {
  Lexer lexer(source, "test.rac");
  auto tokens = lexer.lex();
  if (lexer.has_errors()) {
    return {nullptr, {}, false};
  }

  Parser parser(std::move(tokens));
  auto parse_result = parser.parse();
  if (!parse_result.has_value()) {
    return {nullptr, {}, false};
  }

  auto ast = std::move(parse_result.value());
  SemanticAnalyzer analyzer;
  auto sem_result = analyzer.analyze(*ast);

  if (sem_result.has_value()) {
    return {std::move(ast), {}, true};
  } else {
    return {std::move(ast), std::move(sem_result.error()), false};
  }
}

TEST_CASE("Semantic analyzer accepts valid main function", "[semantic]") {
  auto result = analyze_source(R"(
        fun main(): i32 {
            return 42;
        }
    )");
  REQUIRE(result.success);
}

TEST_CASE("Semantic analyzer accepts variable declaration with initializer",
          "[semantic]") {
  auto result = analyze_source(R"(
        fun main(): i32 {
            let x: i32 = 42;
            return x;
        }
    )");
  REQUIRE(result.success);
}

TEST_CASE("Semantic analyzer accepts uninitialized variable", "[semantic]") {
  auto result = analyze_source(R"(
        fun main(): i32 {
            let x: i32;
            return 0;
        }
    )");
  REQUIRE(result.success);
}

TEST_CASE("Semantic analyzer accepts binary expressions", "[semantic]") {
  auto result = analyze_source(R"(
        fun main(): i32 {
            return 10 + 20;
        }
    )");
  REQUIRE(result.success);
}

TEST_CASE("Semantic analyzer accepts complex expressions", "[semantic]") {
  auto result = analyze_source(R"(
        fun main(): i32 {
            let x: i32 = 10;
            let y: i32 = 20;
            let result: i32 = x + y * 2;
            return result;
        }
    )");
  REQUIRE(result.success);
}

TEST_CASE("Semantic analyzer accepts assignment", "[semantic]") {
  auto result = analyze_source(R"(
        fun main(): i32 {
            let x: i32 = 5;
            x = x + 10;
            return x;
        }
    )");
  REQUIRE(result.success);
}

TEST_CASE("Semantic analyzer detects undefined variable in expression",
          "[semantic]") {
  auto result = analyze_source(R"(
        fun main(): i32 {
            return x;
        }
    )");
  REQUIRE_FALSE(result.success);
  REQUIRE(result.errors.size() >= 1);
  REQUIRE(result.errors[0].message.find("Undefined variable") !=
          std::string::npos);
}

TEST_CASE("Semantic analyzer detects undefined variable in assignment",
          "[semantic]") {
  auto result = analyze_source(R"(
        fun main(): i32 {
            x = 42;
            return 0;
        }
    )");
  REQUIRE_FALSE(result.success);
  REQUIRE(result.errors.size() >= 1);
  REQUIRE(result.errors[0].message.find("Undefined variable") !=
          std::string::npos);
}

TEST_CASE("Semantic analyzer detects duplicate variable in same scope",
          "[semantic]") {
  auto result = analyze_source(R"(
        fun main(): i32 {
            let x: i32 = 1;
            let x: i32 = 2;
            return x;
        }
    )");
  REQUIRE_FALSE(result.success);
  REQUIRE(result.errors.size() >= 1);
  REQUIRE(result.errors[0].message.find("already defined") !=
          std::string::npos);
}

TEST_CASE("Semantic analyzer requires return in non-void function",
          "[semantic]") {
  auto result = analyze_source(R"(
        fun get_value(): i32 {
        }
    )");
  REQUIRE_FALSE(result.success);
  REQUIRE(result.errors.size() >= 1);
  REQUIRE(result.errors[0].message.find("return statement") !=
          std::string::npos);
}

TEST_CASE("Semantic analyzer detects duplicate functions", "[semantic]") {
  auto result = analyze_source(R"(
        fun main(): i32 {
            return 1;
        }
        fun main(): i32 {
            return 2;
        }
    )");
  REQUIRE_FALSE(result.success);
  REQUIRE(result.errors.size() >= 1);
  REQUIRE(result.errors[0].message.find("already defined") !=
          std::string::npos);
}

TEST_CASE("Semantic analyzer fills resolved_type for literals", "[semantic]") {
  auto result = analyze_source(R"(
        fun main(): i32 {
            return 42;
        }
    )");
  REQUIRE(result.success);

  auto &func = result.ast->functions[0];
  auto *ret = static_cast<ReturnStmt *>(func->body->statements[0].get());
  auto *lit = static_cast<IntegerLiteral *>(ret->value.get());
  REQUIRE(lit->resolved_type != nullptr);
  REQUIRE(lit->resolved_type->kind == Type::Kind::I32);
}

TEST_CASE("Semantic analyzer fills resolved_type for binary expressions",
          "[semantic]") {
  auto result = analyze_source(R"(
        fun main(): i32 {
            return 10 + 20;
        }
    )");
  REQUIRE(result.success);

  auto &func = result.ast->functions[0];
  auto *ret = static_cast<ReturnStmt *>(func->body->statements[0].get());
  REQUIRE(ret->value->resolved_type != nullptr);
  REQUIRE(ret->value->resolved_type->kind == Type::Kind::I32);
}

TEST_CASE("Semantic analyzer fills resolved_type for identifiers",
          "[semantic]") {
  auto result = analyze_source(R"(
        fun main(): i32 {
            let x: i32 = 10;
            return x;
        }
    )");
  REQUIRE(result.success);

  auto &func = result.ast->functions[0];
  auto *ret = static_cast<ReturnStmt *>(func->body->statements[1].get());
  REQUIRE(ret->value->resolved_type != nullptr);
  REQUIRE(ret->value->resolved_type->kind == Type::Kind::I32);
}

TEST_CASE("Semantic analyzer accepts expression statement", "[semantic]") {
  auto result = analyze_source(R"(
        fun main(): i32 {
            42 + 1;
            return 0;
        }
    )");
  REQUIRE(result.success);
}
