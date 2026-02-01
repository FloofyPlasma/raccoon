#include <catch2/catch_test_macros.hpp>
#include "raccoon/Parser.hpp"
#include "raccoon/Token.hpp"


TEST_CASE("Parser parses empty main function", "[parser]") {
    std::vector<Token> tokens = {
        {TokenType::FUN, "fun", {"test.rac", 1, 1}},
        {TokenType::IDENTIFIER, "main", {"test.rac", 1, 5}},
        {TokenType::LEFT_PAREN, "(", {"test.rac", 1, 9}},
        {TokenType::RIGHT_PAREN, ")", {"test.rac", 1, 10}},
        {TokenType::COLON, ":", {"test.rac", 1, 11}},
        {TokenType::I32, "i32", {"test.rac", 1, 13}},
        {TokenType::LEFT_BRACE, "{", {"test.rac", 1, 17}},
        {TokenType::RIGHT_BRACE, "}", {"test.rac", 2, 1}},
        {TokenType::END_OF_FILE, "", {"test.rac", 2, 2}}
    };
    
    Parser parser(std::move(tokens));
    auto result = parser.parse();
    
    REQUIRE(result.has_value());
    auto& program = result.value();
    REQUIRE(program->functions.size() == 1);
    REQUIRE(program->functions[0]->name == "main");
    REQUIRE(program->functions[0]->return_type->kind == Type::Kind::I32);
    REQUIRE(program->functions[0]->body->statements.empty());
}

TEST_CASE("Parser parses return statement", "[parser]") {
    std::vector<Token> tokens = {
        {TokenType::FUN, "fun", {"test.rac", 1, 1}},
        {TokenType::IDENTIFIER, "main", {"test.rac", 1, 5}},
        {TokenType::LEFT_PAREN, "(", {"test.rac", 1, 9}},
        {TokenType::RIGHT_PAREN, ")", {"test.rac", 1, 10}},
        {TokenType::COLON, ":", {"test.rac", 1, 11}},
        {TokenType::I32, "i32", {"test.rac", 1, 13}},
        {TokenType::LEFT_BRACE, "{", {"test.rac", 1, 17}},
        {TokenType::RETURN, "return", {"test.rac", 2, 5}},
        {TokenType::INTEGER, "42", {"test.rac", 2, 12}, 42},
        {TokenType::SEMICOLON, ";", {"test.rac", 2, 14}},
        {TokenType::RIGHT_BRACE, "}", {"test.rac", 3, 1}},
        {TokenType::END_OF_FILE, "", {"test.rac", 3, 2}}
    };
    
    Parser parser(std::move(tokens));
    auto result = parser.parse();
    
    REQUIRE(result.has_value());
    auto& program = result.value();
    REQUIRE(program->functions.size() == 1);
    
    auto& func = program->functions[0];
    REQUIRE(func->body->statements.size() == 1);
    REQUIRE(func->body->statements[0]->kind == Statement::Kind::RETURN);
    
    auto* ret_stmt = static_cast<ReturnStmt*>(func->body->statements[0].get());
    REQUIRE(ret_stmt != nullptr);
    REQUIRE(ret_stmt->value != nullptr);
    REQUIRE(ret_stmt->value->kind == Expression::Kind::INTEGER_LITERAL);
    
    auto* int_lit = static_cast<IntegerLiteral*>(ret_stmt->value.get());
    REQUIRE(int_lit->value == 42);
}

TEST_CASE("Parser reports missing semicolon", "[parser]") {
    std::vector<Token> tokens = {
        {TokenType::FUN, "fun", {"test.rac", 1, 1}},
        {TokenType::IDENTIFIER, "main", {"test.rac", 1, 5}},
        {TokenType::LEFT_PAREN, "(", {"test.rac", 1, 9}},
        {TokenType::RIGHT_PAREN, ")", {"test.rac", 1, 10}},
        {TokenType::COLON, ":", {"test.rac", 1, 11}},
        {TokenType::I32, "i32", {"test.rac", 1, 13}},
        {TokenType::LEFT_BRACE, "{", {"test.rac", 1, 17}},
        {TokenType::RETURN, "return", {"test.rac", 2, 5}},
        {TokenType::INTEGER, "42", {"test.rac", 2, 12}, 42},
        // Missing SEMICOLON
        {TokenType::RIGHT_BRACE, "}", {"test.rac", 3, 1}},
        {TokenType::END_OF_FILE, "", {"test.rac", 3, 2}}
    };
    
    Parser parser(std::move(tokens));
    auto result = parser.parse();
    
    REQUIRE_FALSE(result.has_value());
    auto& errors = result.error();
    REQUIRE(errors.size() >= 1);
    REQUIRE(errors[0].message.find("';'") != std::string::npos);
}

TEST_CASE("Parser reports unexpected token", "[parser]") {
    std::vector<Token> tokens = {
        {TokenType::RETURN, "return", {"test.rac", 1, 1}}, // Should start with 'fun'
        {TokenType::INTEGER, "42", {"test.rac", 1, 8}, 42},
        {TokenType::END_OF_FILE, "", {"test.rac", 1, 10}}
    };
    
    Parser parser(std::move(tokens));
    auto result = parser.parse();
    
    REQUIRE_FALSE(result.has_value());
    auto& errors = result.error();
    REQUIRE_FALSE(errors.empty());
}

TEST_CASE("Parser handles multiple return statements", "[parser]") {
    std::vector<Token> tokens = {
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
        {TokenType::RETURN, "return", {"test.rac", 3, 5}},
        {TokenType::INTEGER, "2", {"test.rac", 3, 12}, 2},
        {TokenType::SEMICOLON, ";", {"test.rac", 3, 13}},
        {TokenType::RIGHT_BRACE, "}", {"test.rac", 4, 1}},
        {TokenType::END_OF_FILE, "", {"test.rac", 4, 2}}
    };
    
    Parser parser(std::move(tokens));
    auto result = parser.parse();
    
    REQUIRE(result.has_value());
    auto& program = result.value();
    auto& func = program->functions[0];
    REQUIRE(func->body->statements.size() == 2);
}

TEST_CASE("Parser uses visitor pattern", "[parser]") {
    std::vector<Token> tokens = {
        {TokenType::FUN, "fun", {"test.rac", 1, 1}},
        {TokenType::IDENTIFIER, "main", {"test.rac", 1, 5}},
        {TokenType::LEFT_PAREN, "(", {"test.rac", 1, 9}},
        {TokenType::RIGHT_PAREN, ")", {"test.rac", 1, 10}},
        {TokenType::COLON, ":", {"test.rac", 1, 11}},
        {TokenType::I32, "i32", {"test.rac", 1, 13}},
        {TokenType::LEFT_BRACE, "{", {"test.rac", 1, 17}},
        {TokenType::RETURN, "return", {"test.rac", 2, 5}},
        {TokenType::INTEGER, "42", {"test.rac", 2, 12}, 42},
        {TokenType::SEMICOLON, ";", {"test.rac", 2, 14}},
        {TokenType::RIGHT_BRACE, "}", {"test.rac", 3, 1}},
        {TokenType::END_OF_FILE, "", {"test.rac", 3, 2}}
    };
    
    Parser parser(std::move(tokens));
    auto result = parser.parse();
    
    REQUIRE(result.has_value());
    
    // Test visitor pattern works
    class TestVisitor : public ASTVisitor {
    public:
        int function_count = 0;
        int return_count = 0;
        int literal_count = 0;
        
        void visit(ProgramNode& node) override {
            for (auto& func : node.functions) {
                func->accept(*this);
            }
        }
        
        void visit(FunctionDecl& node) override {
            function_count++;
            if (node.body) {
                node.body->accept(*this);
            }
        }
        
        void visit(BlockStmt& node) override {
            for (auto& stmt : node.statements) {
                stmt->accept(*this);
            }
        }
        
        void visit(ReturnStmt& node) override {
            return_count++;
            if (node.value) {
                node.value->accept(*this);
            }
        }
        
        void visit(IntegerLiteral& node) override {
            (void)node;
            literal_count++;
        }
    };
    
    TestVisitor visitor;
    result.value()->accept(visitor);
    
    REQUIRE(visitor.function_count == 1);
    REQUIRE(visitor.return_count == 1);
    REQUIRE(visitor.literal_count == 1);
}