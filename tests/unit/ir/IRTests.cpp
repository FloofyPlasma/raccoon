#include <catch2/catch_test_macros.hpp>

#include "raccoon/IRGenerator.hpp"
#include "raccoon/Lexer.hpp"
#include "raccoon/Parser.hpp"
#include "raccoon/SemanticAnalyzer.hpp"

std::unique_ptr<IRModule> source_to_ir(const std::string &source) {
  Lexer lexer(source, "test.rac");
  auto tokens = lexer.lex();
  REQUIRE_FALSE(lexer.has_errors());

  Parser parser(std::move(tokens));
  auto parse_result = parser.parse();
  REQUIRE(parse_result.has_value());

  auto ast = std::move(parse_result.value());
  SemanticAnalyzer analyzer;
  auto sem = analyzer.analyze(*ast);
  REQUIRE(sem.has_value());

  IRGenerator gen;
  return gen.generate(*ast, "test");
}

int count_opcode(const IRFunction& func, IRInstruction::OpCode op) {
  int count = 0;
  for (auto& block : func.basic_blocks)
    for (auto& instr : block->instructions)
      if (instr->opcode == op) count++;
  return count;
}

TEST_CASE("IR generator creates module", "[ir]") {
  auto ir = source_to_ir(R"(
        fun main(): i32 { return 42; }
    )");
  REQUIRE(ir != nullptr);
  REQUIRE(ir->name == "test");
  REQUIRE(ir->functions.size() == 1);
}

TEST_CASE("IR generator emits return constant", "[ir]") {
  auto ir = source_to_ir(R"(
        fun main(): i32 { return 42; }
    )");

  auto &block = ir->functions[0]->basic_blocks[0];
  REQUIRE(block->instructions.size() == 1);

  auto &ret = block->instructions[0];
  REQUIRE(ret->opcode == IRInstruction::OpCode::RET);
  REQUIRE(ret->operands.size() == 1);
  REQUIRE(ret->operands[0].kind == IRValue::Kind::CONSTANT);
  REQUIRE(ret->operands[0].int_const == 42);
}

TEST_CASE("IR generator emits binary arithmetic", "[ir]") {
  auto ir = source_to_ir(R"(
        fun main(): i32 { return 10 + 20; }
    )");

  auto &block = ir->functions[0]->basic_blocks[0];
  // Should have: ADD, RET
  REQUIRE(block->instructions.size() == 2);

  auto &add = block->instructions[0];
  REQUIRE(add->opcode == IRInstruction::OpCode::ADD);
  REQUIRE(add->operands.size() == 2);
  REQUIRE(add->operands[0].int_const == 10);
  REQUIRE(add->operands[1].int_const == 20);
  REQUIRE(add->result.kind == IRValue::Kind::REGISTER);

  auto &ret = block->instructions[1];
  REQUIRE(ret->opcode == IRInstruction::OpCode::RET);
  REQUIRE(ret->operands[0].kind == IRValue::Kind::REGISTER);
}

TEST_CASE("IR generator emits all arithmetic operators", "[ir]") {
  SECTION("subtraction") {
    auto ir = source_to_ir("fun main(): i32 { return 10 - 3; }");
    REQUIRE(ir->functions[0]->basic_blocks[0]->instructions[0]->opcode ==
            IRInstruction::OpCode::SUB);
  }
  SECTION("multiplication") {
    auto ir = source_to_ir("fun main(): i32 { return 10 * 3; }");
    REQUIRE(ir->functions[0]->basic_blocks[0]->instructions[0]->opcode ==
            IRInstruction::OpCode::MUL);
  }
  SECTION("division") {
    auto ir = source_to_ir("fun main(): i32 { return 10 / 3; }");
    REQUIRE(ir->functions[0]->basic_blocks[0]->instructions[0]->opcode ==
            IRInstruction::OpCode::SDIV);
  }
}

TEST_CASE("IR generator emits alloca and store for variable decl", "[ir]") {
  auto ir = source_to_ir(R"(
        fun main(): i32 {
            let x: i32 = 42;
            return 0;
        }
    )");

  auto &block = ir->functions[0]->basic_blocks[0];
  // ALLOCA, STORE, RET
  REQUIRE(block->instructions.size() >= 3);

  REQUIRE(block->instructions[0]->opcode == IRInstruction::OpCode::ALLOCA);
  REQUIRE(block->instructions[1]->opcode == IRInstruction::OpCode::STORE);
  // Store operands: value, pointer
  REQUIRE(block->instructions[1]->operands.size() == 2);
  REQUIRE(block->instructions[1]->operands[0].int_const == 42);
}

TEST_CASE("IR generator emits alloca without store for uninitialized var",
          "[ir]") {
  auto ir = source_to_ir(R"(
        fun main(): i32 {
            let x: i32;
            return 0;
        }
    )");

  auto &block = ir->functions[0]->basic_blocks[0];
  // ALLOCA, RET (no STORE for uninitialized)
  REQUIRE(block->instructions.size() == 2);
  REQUIRE(block->instructions[0]->opcode == IRInstruction::OpCode::ALLOCA);
  REQUIRE(block->instructions[1]->opcode == IRInstruction::OpCode::RET);
}

TEST_CASE("IR generator emits load for variable use", "[ir]") {
  auto ir = source_to_ir(R"(
        fun main(): i32 {
            let x: i32 = 42;
            return x;
        }
    )");

  auto &block = ir->functions[0]->basic_blocks[0];
  // ALLOCA, STORE, LOAD, RET
  REQUIRE(block->instructions.size() == 4);
  REQUIRE(block->instructions[0]->opcode == IRInstruction::OpCode::ALLOCA);
  REQUIRE(block->instructions[1]->opcode == IRInstruction::OpCode::STORE);
  REQUIRE(block->instructions[2]->opcode == IRInstruction::OpCode::LOAD);
  REQUIRE(block->instructions[3]->opcode == IRInstruction::OpCode::RET);
}

TEST_CASE("IR generator emits store for assignment", "[ir]") {
  auto ir = source_to_ir(R"(
        fun main(): i32 {
            let x: i32 = 5;
            x = 10;
            return x;
        }
    )");

  auto &block = ir->functions[0]->basic_blocks[0];
  // ALLOCA, STORE(5), STORE(10), LOAD, RET
  REQUIRE(block->instructions.size() == 5);
  REQUIRE(block->instructions[0]->opcode == IRInstruction::OpCode::ALLOCA);
  REQUIRE(block->instructions[1]->opcode == IRInstruction::OpCode::STORE);
  REQUIRE(block->instructions[2]->opcode == IRInstruction::OpCode::STORE);
  REQUIRE(block->instructions[3]->opcode == IRInstruction::OpCode::LOAD);
  REQUIRE(block->instructions[4]->opcode == IRInstruction::OpCode::RET);
}

TEST_CASE("IR generator handles complex expression with variables", "[ir]") {
  auto ir = source_to_ir(R"(
        fun main(): i32 {
            let x: i32 = 10;
            let y: i32 = 20;
            return x + y;
        }
    )");

  auto &block = ir->functions[0]->basic_blocks[0];
  // x: ALLOCA, STORE
  // y: ALLOCA, STORE
  // return x+y: LOAD(x), LOAD(y), ADD, RET
  REQUIRE(block->instructions.size() == 8);

  // Check last 3 instructions are LOAD, LOAD, ADD, RET
  auto size = block->instructions.size();
  REQUIRE(block->instructions[size - 4]->opcode == IRInstruction::OpCode::LOAD);
  REQUIRE(block->instructions[size - 3]->opcode == IRInstruction::OpCode::LOAD);
  REQUIRE(block->instructions[size - 2]->opcode == IRInstruction::OpCode::ADD);
  REQUIRE(block->instructions[size - 1]->opcode == IRInstruction::OpCode::RET);
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

TEST_CASE("IR: comparison generates icmp", "[ir]") {
    auto ir = source_to_ir("fun f(): bool { return 1 < 2; }");
    REQUIRE(count_opcode(*ir->functions[0], IRInstruction::OpCode::ICMP_SLT) == 1);
}

TEST_CASE("IR: all comparison opcodes", "[ir]") {
    SECTION("==") {
        auto ir = source_to_ir("fun f(): bool { return 1 == 2; }");
        REQUIRE(count_opcode(*ir->functions[0], IRInstruction::OpCode::ICMP_EQ) == 1);
    }
    SECTION("!=") {
        auto ir = source_to_ir("fun f(): bool { return 1 != 2; }");
        REQUIRE(count_opcode(*ir->functions[0], IRInstruction::OpCode::ICMP_NE) == 1);
    }
    SECTION(">") {
        auto ir = source_to_ir("fun f(): bool { return 1 > 2; }");
        REQUIRE(count_opcode(*ir->functions[0], IRInstruction::OpCode::ICMP_SGT) == 1);
    }
    SECTION("<=") {
        auto ir = source_to_ir("fun f(): bool { return 1 <= 2; }");
        REQUIRE(count_opcode(*ir->functions[0], IRInstruction::OpCode::ICMP_SLE) == 1);
    }
    SECTION(">=") {
        auto ir = source_to_ir("fun f(): bool { return 1 >= 2; }");
        REQUIRE(count_opcode(*ir->functions[0], IRInstruction::OpCode::ICMP_SGE) == 1);
    }
}

TEST_CASE("IR: logical and", "[ir]") {
    auto ir = source_to_ir("fun f(): bool { return true && false; }");
    REQUIRE(count_opcode(*ir->functions[0], IRInstruction::OpCode::AND) == 1);
}

TEST_CASE("IR: logical or", "[ir]") {
    auto ir = source_to_ir("fun f(): bool { return true || false; }");
    REQUIRE(count_opcode(*ir->functions[0], IRInstruction::OpCode::OR) == 1);
}

TEST_CASE("IR: unary minus", "[ir]") {
    auto ir = source_to_ir("fun main(): i32 { return -42; }");
    REQUIRE(count_opcode(*ir->functions[0], IRInstruction::OpCode::NEG) == 1);
}

TEST_CASE("IR: unary not", "[ir]") {
    auto ir = source_to_ir("fun f(): bool { return !true; }");
    REQUIRE(count_opcode(*ir->functions[0], IRInstruction::OpCode::NOT) == 1);
}

TEST_CASE("IR: if creates multiple basic blocks", "[ir]") {
    auto ir = source_to_ir(R"(
        fun main(): i32 {
            if (true) { return 1; }
            return 0;
        }
    )");
    // entry, then, merge (at minimum)
    REQUIRE(ir->functions[0]->basic_blocks.size() >= 3);
    REQUIRE(count_opcode(*ir->functions[0], IRInstruction::OpCode::BR_COND) == 1);
}

TEST_CASE("IR: if/else creates then and else blocks", "[ir]") {
    auto ir = source_to_ir(R"(
        fun main(): i32 {
            if (true) { return 1; }
            else { return 0; }
        }
    )");
    // entry, then, else, merge
    REQUIRE(ir->functions[0]->basic_blocks.size() >= 3);
    REQUIRE(count_opcode(*ir->functions[0], IRInstruction::OpCode::BR_COND) == 1);
}

TEST_CASE("IR: while creates loop blocks", "[ir]") {
    auto ir = source_to_ir(R"(
        fun main(): i32 {
            let x: i32 = 0;
            while (x < 10) { x = x + 1; }
            return x;
        }
    )");
    // entry, while.cond, while.body, while.exit
    REQUIRE(ir->functions[0]->basic_blocks.size() >= 4);
    REQUIRE(count_opcode(*ir->functions[0], IRInstruction::OpCode::BR_COND) >= 1);
}

TEST_CASE("IR: break generates branch to exit", "[ir]") {
    auto ir = source_to_ir(R"(
        fun main(): i32 {
            while (true) { break; }
            return 0;
        }
    )");
    // The break should generate a BR to exit label
    REQUIRE(count_opcode(*ir->functions[0], IRInstruction::OpCode::BR) >= 1);
}

TEST_CASE("IR: for loop creates blocks", "[ir]") {
    auto ir = source_to_ir(R"(
        fun main(): i32 {
            let sum: i32 = 0;
            for (let i: i32 = 0; i < 10; i = i + 1) {
                sum = sum + i;
            }
            return sum;
        }
    )");
    // entry, for.cond, for.body, for.incr, for.exit
    REQUIRE(ir->functions[0]->basic_blocks.size() >= 5);
}

TEST_CASE("IR: bool constant", "[ir]") {
    auto ir = source_to_ir("fun f(): bool { return true; }");
    auto& block = ir->functions[0]->basic_blocks[0];
    auto& ret = block->instructions.back();
    REQUIRE(ret->opcode == IRInstruction::OpCode::RET);
    REQUIRE(ret->operands[0].kind == IRValue::Kind::CONSTANT);
    REQUIRE(ret->operands[0].type.kind == IRType::Kind::BOOL);
    REQUIRE(ret->operands[0].int_const == 1);
}

TEST_CASE("IR value factory methods", "[ir]") {
    auto reg = IRValue::make_register(IRType(IRType::Kind::BOOL), "flag");
    REQUIRE(reg.kind == IRValue::Kind::REGISTER);
    REQUIRE(reg.type.kind == IRType::Kind::BOOL);

    auto c = IRValue::make_constant(IRType(IRType::Kind::BOOL), 1);
    REQUIRE(c.kind == IRValue::Kind::CONSTANT);
    REQUIRE(c.int_const == 1);
}