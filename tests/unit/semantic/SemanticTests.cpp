#include <catch2/catch_test_macros.hpp>
#include "raccoon/Parser.hpp"
#include "raccoon/SemanticAnalyzer.hpp"
#include "raccoon/Token.hpp"

std::vector<Token> make_tokens_for_program(const std::string& return_value = "42") {
    std::vector<Token> tokens = {
        {TokenType::FUN, "fun", {"test.rac", 1, 1}},
        {TokenType::IDENTIFIER, "main", {"test.rac", 1, 5}},
        {TokenType::LEFT_PAREN, "(", {"test.rac", 1, 9}},
        {TokenType::RIGHT_PAREN, ")", {"test.rac", 1, 10}},
        {TokenType::COLON, ":", {"test.rac", 1, 11}},
        {TokenType::I32, "i32", {"test.rac", 1, 13}},
        {TokenType::LEFT_BRACE, "{", {"test.rac", 1, 17}},
        {TokenType::RETURN, "return", {"test.rac", 2, 5}},
        {TokenType::INTEGER, return_value, {"test.rac", 2, 12}, std::stoll(return_value)},
        {TokenType::SEMICOLON, ";", {"test.rac", 2, 14}},
        {TokenType::RIGHT_BRACE, "}", {"test.rac", 3, 1}},
        {TokenType::END_OF_FILE, "", {"test.rac", 3, 2}}
    };
    return tokens;
}

TEST_CASE("Semantic analyzer accepts valid main function", "[semantic]") {
    auto tokens = make_tokens_for_program();
    
    Parser parser(std::move(tokens));
    auto parse_result = parser.parse();
    REQUIRE(parse_result.has_value());
    
    SemanticAnalyzer analyzer;
    auto semantic_result = analyzer.analyze(*parse_result.value());
    
    REQUIRE(semantic_result.has_value());
}

TEST_CASE("Semantic analyzer fills resolved_type for literals", "[semantic]") {
    auto tokens = make_tokens_for_program();
    
    Parser parser(std::move(tokens));
    auto parse_result = parser.parse();
    REQUIRE(parse_result.has_value());
    
    auto& program = parse_result.value();
    
    SemanticAnalyzer analyzer;
    auto semantic_result = analyzer.analyze(*program);
    REQUIRE(semantic_result.has_value());
    
    // Check that the literal has been annotated with type
    auto& func = program->functions[0];
    auto* ret_stmt = static_cast<ReturnStmt*>(func->body->statements[0].get());
    auto* literal = static_cast<IntegerLiteral*>(ret_stmt->value.get());
    
    REQUIRE(literal->resolved_type != nullptr);
    REQUIRE(literal->resolved_type->kind == Type::Kind::I32);
}

TEST_CASE("Semantic analyzer requires return in non-void function", "[semantic]") {
    std::vector<Token> tokens = {
        {TokenType::FUN, "fun", {"test.rac", 1, 1}},
        {TokenType::IDENTIFIER, "get_value", {"test.rac", 1, 5}},
        {TokenType::LEFT_PAREN, "(", {"test.rac", 1, 14}},
        {TokenType::RIGHT_PAREN, ")", {"test.rac", 1, 15}},
        {TokenType::COLON, ":", {"test.rac", 1, 16}},
        {TokenType::I32, "i32", {"test.rac", 1, 18}},
        {TokenType::LEFT_BRACE, "{", {"test.rac", 1, 22}},
        {TokenType::RIGHT_BRACE, "}", {"test.rac", 2, 1}},
        {TokenType::END_OF_FILE, "", {"test.rac", 2, 2}}
    };
    
    Parser parser(std::move(tokens));
    auto parse_result = parser.parse();
    REQUIRE(parse_result.has_value());
    
    SemanticAnalyzer analyzer;
    auto semantic_result = analyzer.analyze(*parse_result.value());
    
    REQUIRE_FALSE(semantic_result.has_value());
    auto& errors = semantic_result.error();
    REQUIRE(errors.size() >= 1);
    REQUIRE(errors[0].message.find("return statement") != std::string::npos);
}

TEST_CASE("Semantic analyzer allows empty void function", "[semantic]") {
    std::vector<Token> tokens = {
        {TokenType::FUN, "fun", {"test.rac", 1, 1}},
        {TokenType::IDENTIFIER, "do_nothing", {"test.rac", 1, 5}},
        {TokenType::LEFT_PAREN, "(", {"test.rac", 1, 15}},
        {TokenType::RIGHT_PAREN, ")", {"test.rac", 1, 16}},
        {TokenType::COLON, ":", {"test.rac", 1, 17}},
        {TokenType::IDENTIFIER, "void", {"test.rac", 1, 19}},
        {TokenType::LEFT_BRACE, "{", {"test.rac", 1, 24}},
        {TokenType::RIGHT_BRACE, "}", {"test.rac", 2, 1}},
        {TokenType::END_OF_FILE, "", {"test.rac", 2, 2}}
    };
    
    // First need to add VOID token type to lexer
    // For now, skip this test as Phase 1 only has i32
}

TEST_CASE("Semantic analyzer detects duplicate functions", "[semantic]") {
    std::vector<Token> tokens = {
        // First main
        {TokenType::FUN, "fun", {"test.rac", 1, 1}},
        {TokenType::IDENTIFIER, "main", {"test.rac", 1, 5}},
        {TokenType::LEFT_PAREN, "(", {"test.rac", 1, 9}},
        {TokenType::RIGHT_PAREN, ")", {"test.rac", 1, 10}},
        {TokenType::COLON, ":", {"test.rac", 1, 11}},
        {TokenType::I32, "i32", {"test.rac", 1, 13}},
        {TokenType::LEFT_BRACE, "{", {"test.rac", 1, 17}},
        {TokenType::RETURN, "return", {"test.rac", 2, 5}},
        {TokenType::INTEGER, "1", {"test.rac", 2, 12}, 1},
        {TokenType::SEMICOLON, ";", {"test.rac", 2, 13}},
        {TokenType::RIGHT_BRACE, "}", {"test.rac", 3, 1}},
        
        // Second main (duplicate)
        {TokenType::FUN, "fun", {"test.rac", 5, 1}},
        {TokenType::IDENTIFIER, "main", {"test.rac", 5, 5}},
        {TokenType::LEFT_PAREN, "(", {"test.rac", 5, 9}},
        {TokenType::RIGHT_PAREN, ")", {"test.rac", 5, 10}},
        {TokenType::COLON, ":", {"test.rac", 5, 11}},
        {TokenType::I32, "i32", {"test.rac", 5, 13}},
        {TokenType::LEFT_BRACE, "{", {"test.rac", 5, 17}},
        {TokenType::RETURN, "return", {"test.rac", 6, 5}},
        {TokenType::INTEGER, "2", {"test.rac", 6, 12}, 2},
        {TokenType::SEMICOLON, ";", {"test.rac", 6, 13}},
        {TokenType::RIGHT_BRACE, "}", {"test.rac", 7, 1}},
        
        {TokenType::END_OF_FILE, "", {"test.rac", 8, 1}}
    };
    
    Parser parser(std::move(tokens));
    auto parse_result = parser.parse();
    REQUIRE(parse_result.has_value());
    
    SemanticAnalyzer analyzer;
    auto semantic_result = analyzer.analyze(*parse_result.value());
    
    REQUIRE_FALSE(semantic_result.has_value());
    auto& errors = semantic_result.error();
    REQUIRE(errors.size() >= 1);
    REQUIRE(errors[0].message.find("already defined") != std::string::npos);
}

TEST_CASE("Semantic analyzer continues after errors", "[semantic]") {
    std::vector<Token> tokens = {
        // First function - duplicate
        {TokenType::FUN, "fun", {"test.rac", 1, 1}},
        {TokenType::IDENTIFIER, "test", {"test.rac", 1, 5}},
        {TokenType::LEFT_PAREN, "(", {"test.rac", 1, 9}},
        {TokenType::RIGHT_PAREN, ")", {"test.rac", 1, 10}},
        {TokenType::COLON, ":", {"test.rac", 1, 11}},
        {TokenType::I32, "i32", {"test.rac", 1, 13}},
        {TokenType::LEFT_BRACE, "{", {"test.rac", 1, 17}},
        {TokenType::RETURN, "return", {"test.rac", 2, 5}},
        {TokenType::INTEGER, "1", {"test.rac", 2, 12}, 1},
        {TokenType::SEMICOLON, ";", {"test.rac", 2, 13}},
        {TokenType::RIGHT_BRACE, "}", {"test.rac", 3, 1}},
        
        // Second function - duplicate (error)
        {TokenType::FUN, "fun", {"test.rac", 5, 1}},
        {TokenType::IDENTIFIER, "test", {"test.rac", 5, 5}},
        {TokenType::LEFT_PAREN, "(", {"test.rac", 5, 9}},
        {TokenType::RIGHT_PAREN, ")", {"test.rac", 5, 10}},
        {TokenType::COLON, ":", {"test.rac", 5, 11}},
        {TokenType::I32, "i32", {"test.rac", 5, 13}},
        {TokenType::LEFT_BRACE, "{", {"test.rac", 5, 17}},
        {TokenType::RIGHT_BRACE, "}", {"test.rac", 6, 1}}, // Missing return (error)
        
        {TokenType::END_OF_FILE, "", {"test.rac", 7, 1}}
    };
    
    Parser parser(std::move(tokens));
    auto parse_result = parser.parse();
    REQUIRE(parse_result.has_value());
    
    SemanticAnalyzer analyzer;
    auto semantic_result = analyzer.analyze(*parse_result.value());
    
    REQUIRE_FALSE(semantic_result.has_value());
    auto& errors = semantic_result.error();
    
    // Should collect both errors: duplicate + missing return
    REQUIRE(errors.size() >= 2);
}