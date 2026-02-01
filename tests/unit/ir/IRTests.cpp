#include <catch2/catch_test_macros.hpp>
#include "raccoon/Parser.hpp"
#include "raccoon/SemanticAnalyzer.hpp"
#include "raccoon/IRGenerator.hpp"
#include "raccoon/Token.hpp"

// Helper to create minimal valid program tokens
std::vector<Token> make_test_tokens() {
    return {
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
}

std::unique_ptr<ProgramNode> parse_and_analyze(std::vector<Token> tokens) {
    Parser parser(std::move(tokens));
    auto parse_result = parser.parse();
    REQUIRE(parse_result.has_value());
    
    auto ast = std::move(parse_result.value());
    
    SemanticAnalyzer analyzer;
    auto semantic_result = analyzer.analyze(*ast);
    REQUIRE(semantic_result.has_value());
    
    return ast;
}

TEST_CASE("IR generator creates module", "[ir]") {
    auto ast = parse_and_analyze(make_test_tokens());
    
    IRGenerator ir_gen;
    auto ir_module = ir_gen.generate(*ast, "test");
    
    REQUIRE(ir_module != nullptr);
    REQUIRE(ir_module->name == "test");
    REQUIRE(ir_module->functions.size() == 1);
}

TEST_CASE("IR generator creates function", "[ir]") {
    auto ast = parse_and_analyze(make_test_tokens());
    
    IRGenerator ir_gen;
    auto ir_module = ir_gen.generate(*ast, "test");
    
    auto& func = ir_module->functions[0];
    REQUIRE(func->name == "main");
    REQUIRE(func->return_type.kind == IRType::Kind::I32);
    REQUIRE(func->parameters.empty());  // Phase 1: no parameters
    REQUIRE(func->basic_blocks.size() == 1);
}

TEST_CASE("IR generator creates entry block", "[ir]") {
    auto ast = parse_and_analyze(make_test_tokens());
    
    IRGenerator ir_gen;
    auto ir_module = ir_gen.generate(*ast, "test");
    
    auto& func = ir_module->functions[0];
    auto& block = func->basic_blocks[0];
    
    REQUIRE(block->label == "entry");
    REQUIRE(block->instructions.size() == 1);
}

TEST_CASE("IR generator emits return instruction", "[ir]") {
    auto ast = parse_and_analyze(make_test_tokens());
    
    IRGenerator ir_gen;
    auto ir_module = ir_gen.generate(*ast, "test");
    
    auto& func = ir_module->functions[0];
    auto& block = func->basic_blocks[0];
    auto& instr = block->instructions[0];
    
    REQUIRE(instr->opcode == IRInstruction::OpCode::RET);
    REQUIRE(instr->operands.size() == 1);
    REQUIRE(instr->operands[0].kind == IRValue::Kind::CONSTANT);
    REQUIRE(instr->operands[0].type.kind == IRType::Kind::I32);
    REQUIRE(instr->operands[0].int_const == 42);
}

TEST_CASE("IR generator handles multiple functions", "[ir]") {
    std::vector<Token> tokens = {
        // First function
        {TokenType::FUN, "fun", {"test.rac", 1, 1}},
        {TokenType::IDENTIFIER, "foo", {"test.rac", 1, 5}},
        {TokenType::LEFT_PAREN, "(", {"test.rac", 1, 8}},
        {TokenType::RIGHT_PAREN, ")", {"test.rac", 1, 9}},
        {TokenType::COLON, ":", {"test.rac", 1, 10}},
        {TokenType::I32, "i32", {"test.rac", 1, 12}},
        {TokenType::LEFT_BRACE, "{", {"test.rac", 1, 16}},
        {TokenType::RETURN, "return", {"test.rac", 2, 5}},
        {TokenType::INTEGER, "1", {"test.rac", 2, 12}, 1},
        {TokenType::SEMICOLON, ";", {"test.rac", 2, 13}},
        {TokenType::RIGHT_BRACE, "}", {"test.rac", 3, 1}},
        
        // Second function
        {TokenType::FUN, "fun", {"test.rac", 5, 1}},
        {TokenType::IDENTIFIER, "bar", {"test.rac", 5, 5}},
        {TokenType::LEFT_PAREN, "(", {"test.rac", 5, 8}},
        {TokenType::RIGHT_PAREN, ")", {"test.rac", 5, 9}},
        {TokenType::COLON, ":", {"test.rac", 5, 10}},
        {TokenType::I32, "i32", {"test.rac", 5, 12}},
        {TokenType::LEFT_BRACE, "{", {"test.rac", 5, 16}},
        {TokenType::RETURN, "return", {"test.rac", 6, 5}},
        {TokenType::INTEGER, "2", {"test.rac", 6, 12}, 2},
        {TokenType::SEMICOLON, ";", {"test.rac", 6, 13}},
        {TokenType::RIGHT_BRACE, "}", {"test.rac", 7, 1}},
        
        {TokenType::END_OF_FILE, "", {"test.rac", 8, 1}}
    };
    
    auto ast = parse_and_analyze(std::move(tokens));
    
    IRGenerator ir_gen;
    auto ir_module = ir_gen.generate(*ast, "test");
    
    REQUIRE(ir_module->functions.size() == 2);
    REQUIRE(ir_module->functions[0]->name == "foo");
    REQUIRE(ir_module->functions[1]->name == "bar");
    
    // Check first function returns 1
    auto& foo_instr = ir_module->functions[0]->basic_blocks[0]->instructions[0];
    REQUIRE(foo_instr->operands[0].int_const == 1);
    
    // Check second function returns 2
    auto& bar_instr = ir_module->functions[1]->basic_blocks[0]->instructions[0];
    REQUIRE(bar_instr->operands[0].int_const == 2);
}

TEST_CASE("IR value factory methods work correctly", "[ir]") {
    SECTION("make_register") {
        auto reg = IRValue::make_register(IRType(IRType::Kind::I32), "temp");
        REQUIRE(reg.kind == IRValue::Kind::REGISTER);
        REQUIRE(reg.type.kind == IRType::Kind::I32);
        REQUIRE(reg.name == "temp");
    }
    
    SECTION("make_constant") {
        auto const_val = IRValue::make_constant(IRType(IRType::Kind::I32), 42);
        REQUIRE(const_val.kind == IRValue::Kind::CONSTANT);
        REQUIRE(const_val.type.kind == IRType::Kind::I32);
        REQUIRE(const_val.int_const == 42);
    }
    
    SECTION("make_label") {
        auto label = IRValue::make_label("entry");
        REQUIRE(label.kind == IRValue::Kind::LABEL);
        REQUIRE(label.name == "entry");
    }
}