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

TEST_CASE("Semantic: bool variable", "[semantic]") {
  REQUIRE(analyze_source("fun main(): i32 { let f: bool = true; return 0; }").success);
}

TEST_CASE("Semantic: bool literal false", "[semantic]") {
  REQUIRE(analyze_source("fun f(): bool { return false; }").success);
}

TEST_CASE("Semantic: cannot return i32 from bool function", "[semantic]") {
  auto r = analyze_source("fun f(): bool { return 42; }");
  REQUIRE_FALSE(r.success);
}

TEST_CASE("Semantic: cannot return bool from i32 function", "[semantic]") {
  auto r = analyze_source("fun f(): i32 { return true; }");
  REQUIRE_FALSE(r.success);
}


TEST_CASE("Semantic: comparison returns bool", "[semantic]") {
    auto r = analyze_source("fun f(): bool { return 1 < 2; }");
    REQUIRE(r.success);
}

TEST_CASE("Semantic: all comparison operators", "[semantic]") {
    for (auto op : {"<", ">", "<=", ">="}) {
        auto r = analyze_source(std::string("fun f(): bool { return 1 ") + op + " 2; }");
        REQUIRE(r.success);
    }
}

TEST_CASE("Semantic: equality operators", "[semantic]") {
    REQUIRE(analyze_source("fun f(): bool { return 1 == 2; }").success);
    REQUIRE(analyze_source("fun f(): bool { return 1 != 2; }").success);
    REQUIRE(analyze_source("fun f(): bool { return true == false; }").success);
}

TEST_CASE("Semantic: comparison requires matching types", "[semantic]") {
    // Can't compare i32 with bool
    auto r = analyze_source("fun f(): bool { let x: i32 = 1; let y: bool = true; return x == y; }");
    REQUIRE_FALSE(r.success);
    REQUIRE(r.errors[0].message.find("mismatch") != std::string::npos);
}

TEST_CASE("Semantic: arithmetic on bool is error", "[semantic]") {
    auto r = analyze_source("fun f(): bool { return true + false; }");
    REQUIRE_FALSE(r.success);
    REQUIRE(r.errors[0].message.find("i32") != std::string::npos);
}

TEST_CASE("Semantic: logical and", "[semantic]") {
    REQUIRE(analyze_source("fun f(): bool { return true && false; }").success);
}

TEST_CASE("Semantic: logical or", "[semantic]") {
    REQUIRE(analyze_source("fun f(): bool { return true || false; }").success);
}

TEST_CASE("Semantic: logical on i32 is error", "[semantic]") {
    auto r = analyze_source("fun f(): bool { return 1 && 2; }");
    REQUIRE_FALSE(r.success);
    REQUIRE(r.errors[0].message.find("bool") != std::string::npos);
}

TEST_CASE("Semantic: unary minus", "[semantic]") {
    REQUIRE(analyze_source("fun main(): i32 { return -42; }").success);
}

TEST_CASE("Semantic: unary not", "[semantic]") {
    REQUIRE(analyze_source("fun f(): bool { return !true; }").success);
}

TEST_CASE("Semantic: unary minus on bool is error", "[semantic]") {
    auto r = analyze_source("fun f(): i32 { return -true; }");
    REQUIRE_FALSE(r.success);
}

TEST_CASE("Semantic: unary not on i32 is error", "[semantic]") {
    auto r = analyze_source("fun f(): bool { return !42; }");
    REQUIRE_FALSE(r.success);
}

TEST_CASE("Semantic: if condition must be bool", "[semantic]") {
    auto r = analyze_source("fun main(): i32 { if (42) { return 1; } return 0; }");
    REQUIRE_FALSE(r.success);
    REQUIRE(r.errors[0].message.find("bool") != std::string::npos);
}

TEST_CASE("Semantic: if with bool condition", "[semantic]") {
    REQUIRE(analyze_source("fun main(): i32 { if (true) { return 1; } return 0; }").success);
}

TEST_CASE("Semantic: if/else both returning satisfies return check", "[semantic]") {
    REQUIRE(analyze_source(R"(
        fun main(): i32 {
            if (true) { return 1; }
            else { return 0; }
        }
    )").success);
}

TEST_CASE("Semantic: if without else doesn't satisfy return", "[semantic]") {
    auto r = analyze_source("fun main(): i32 { if (true) { return 1; } }");
    REQUIRE_FALSE(r.success);
}

TEST_CASE("Semantic: while condition must be bool", "[semantic]") {
    auto r = analyze_source("fun main(): i32 { while (42) { break; } return 0; }");
    REQUIRE_FALSE(r.success);
}

TEST_CASE("Semantic: valid while loop", "[semantic]") {
    REQUIRE(analyze_source(R"(
        fun main(): i32 {
            let x: i32 = 0;
            while (x < 10) { x = x + 1; }
            return x;
        }
    )").success);
}

TEST_CASE("Semantic: break outside loop is error", "[semantic]") {
    auto r = analyze_source("fun main(): i32 { break; return 0; }");
    REQUIRE_FALSE(r.success);
    REQUIRE(r.errors[0].message.find("loop") != std::string::npos);
}

TEST_CASE("Semantic: continue outside loop is error", "[semantic]") {
    auto r = analyze_source("fun main(): i32 { continue; return 0; }");
    REQUIRE_FALSE(r.success);
    REQUIRE(r.errors[0].message.find("loop") != std::string::npos);
}

TEST_CASE("Semantic: break inside while is ok", "[semantic]") {
    REQUIRE(analyze_source(R"(
        fun main(): i32 {
            while (true) { break; }
            return 0;
        }
    )").success);
}

TEST_CASE("Semantic: continue inside for is ok", "[semantic]") {
    REQUIRE(analyze_source(R"(
        fun main(): i32 {
            for (let i: i32 = 0; i < 10; i = i + 1) { continue; }
            return 0;
        }
    )").success);
}

TEST_CASE("Semantic: for condition must be bool", "[semantic]") {
    auto r = analyze_source("fun main(): i32 { for (let i: i32 = 0; 42; i = i + 1) { i = i; } return 0; }");
    REQUIRE_FALSE(r.success);
}

TEST_CASE("Semantic: for initializer scoped to loop", "[semantic]") {
    // 'i' should not be accessible after the for loop
    auto r = analyze_source(R"(
        fun main(): i32 {
            for (let i: i32 = 0; i < 10; i = i + 1) { i = i; }
            return i;
        }
    )");
    REQUIRE_FALSE(r.success);
    REQUIRE(r.errors[0].message.find("Undefined") != std::string::npos);
}

TEST_CASE("Semantic: for with empty parts", "[semantic]") {
    REQUIRE(analyze_source(R"(
        fun main(): i32 {
            for (;;) { break; }
            return 0;
        }
    )").success);
}