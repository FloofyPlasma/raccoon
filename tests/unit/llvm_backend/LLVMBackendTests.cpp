#include "raccoon/LLVMBackend.hpp"

#include <catch2/catch_test_macros.hpp>
#include <fstream>
#include <iostream>

TEST_CASE("LLVMBackend generates empty module", "[llvm_backend]") {
  IRModule ir_module("test_empty");
  LLVMBackend backend;

  auto result = backend.generate(ir_module);

  REQUIRE(result.has_value());
  REQUIRE_FALSE(backend.has_errors());
}

TEST_CASE("LLVMBackend translates minimal function", "[llvm_backend]") {
  SECTION("main returns i32 constant") {
    IRModule ir_module("test_main");
    auto func =
        std::make_unique<IRFunction>("main", IRType(IRType::Kind::I32));
    auto block = std::make_unique<IRBasicBlock>("entry");

    auto ret_instr =
        std::make_unique<IRInstruction>(IRInstruction::OpCode::RET);
    ret_instr->operands.push_back(
        IRValue::make_constant(IRType(IRType::Kind::I32), 42));

    block->instructions.push_back(std::move(ret_instr));
    func->basic_blocks.push_back(std::move(block));
    ir_module.functions.push_back(std::move(func));

    LLVMBackend backend;
    auto result = backend.generate(ir_module);

    REQUIRE(result.has_value());
    REQUIRE_FALSE(backend.has_errors());
  }

  SECTION("main returns zero") {
    IRModule ir_module("test_zero");
    auto func =
        std::make_unique<IRFunction>("main", IRType(IRType::Kind::I32));
    auto block = std::make_unique<IRBasicBlock>("entry");

    auto ret_instr =
        std::make_unique<IRInstruction>(IRInstruction::OpCode::RET);
    ret_instr->operands.push_back(
        IRValue::make_constant(IRType(IRType::Kind::I32), 0));

    block->instructions.push_back(std::move(ret_instr));
    func->basic_blocks.push_back(std::move(block));
    ir_module.functions.push_back(std::move(func));

    LLVMBackend backend;
    auto result = backend.generate(ir_module);

    REQUIRE(result.has_value());
    REQUIRE_FALSE(backend.has_errors());
  }
}

TEST_CASE("LLVMBackend handles errors", "[llvm_backend]") {
  SECTION("missing terminator fails verification") {
    IRModule ir_module("test_invalid");
    auto func =
        std::make_unique<IRFunction>("main", IRType(IRType::Kind::I32));
    auto block = std::make_unique<IRBasicBlock>("entry");

    // No terminator instruction - should fail verification
    func->basic_blocks.push_back(std::move(block));
    ir_module.functions.push_back(std::move(func));

    LLVMBackend backend;
    auto result = backend.generate(ir_module);

    // Debug output
    if (result.has_value()) {
      std::cerr << "ERROR: generate() succeeded when it should have failed\n";
      std::cerr << "Backend has errors: " << backend.has_errors() << "\n";
    } else {
      std::cerr << "Good: generate() failed as expected\n";
      for (const auto& err : result.error()) {
        std::cerr << "Error: " << err.message << "\n";
      }
    }

    REQUIRE_FALSE(result.has_value());
    REQUIRE(backend.has_errors());
    REQUIRE_FALSE(backend.get_errors().empty());
    REQUIRE(backend.get_errors()[0].message.find("verification failed") !=
            std::string::npos);
  }
}

TEST_CASE("LLVMBackend emits object file", "[llvm_backend]") {
  SECTION("writes valid .o file") {
    IRModule ir_module("test_emit");
    auto func =
        std::make_unique<IRFunction>("main", IRType(IRType::Kind::I32));
    auto block = std::make_unique<IRBasicBlock>("entry");

    auto ret_instr =
        std::make_unique<IRInstruction>(IRInstruction::OpCode::RET);
    ret_instr->operands.push_back(
        IRValue::make_constant(IRType(IRType::Kind::I32), 42));

    block->instructions.push_back(std::move(ret_instr));
    func->basic_blocks.push_back(std::move(block));
    ir_module.functions.push_back(std::move(func));

    LLVMBackend backend;
    auto gen_result = backend.generate(ir_module);
    REQUIRE(gen_result.has_value());

    // Emit object file
    bool success = backend.emit_object_file("test_emit.o");

    // Debug: print errors if it failed
    if (!success) {
      for (const auto& err : backend.get_errors()) {
        std::cerr << "Error: " << err.message << "\n";
      }
    }

    REQUIRE(success);
    REQUIRE_FALSE(backend.has_errors());

    // Verify file exists and has content
    std::ifstream file("test_emit.o", std::ios::binary | std::ios::ate);
    REQUIRE(file.is_open());
    REQUIRE(file.tellg() > 0); // Not empty

    file.close();
  }

  SECTION("emit without generate fails") {
    LLVMBackend backend;
    bool success = backend.emit_object_file("test_no_module.o");

    REQUIRE_FALSE(success);
    REQUIRE(backend.has_errors());
    REQUIRE(backend.get_errors()[0].message.find("No module") !=
            std::string::npos);
  }
}

TEST_CASE("LLVMBackend emits LLVM IR text", "[llvm_backend]") {
  SECTION("writes valid .ll file") {
    IRModule ir_module("test_ir");
    auto func =
        std::make_unique<IRFunction>("main", IRType(IRType::Kind::I32));
    auto block = std::make_unique<IRBasicBlock>("entry");

    auto ret_instr =
        std::make_unique<IRInstruction>(IRInstruction::OpCode::RET);
    ret_instr->operands.push_back(
        IRValue::make_constant(IRType(IRType::Kind::I32), 99));

    block->instructions.push_back(std::move(ret_instr));
    func->basic_blocks.push_back(std::move(block));
    ir_module.functions.push_back(std::move(func));

    LLVMBackend backend;
    auto gen_result = backend.generate(ir_module);
    REQUIRE(gen_result.has_value());

    // Emit LLVM IR
    bool success = backend.emit_llvm_ir("test_ir.ll");
    REQUIRE(success);
    REQUIRE_FALSE(backend.has_errors());

    // Verify file exists and has content
    std::ifstream file("test_ir.ll");
    REQUIRE(file.is_open());

    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    REQUIRE(!content.empty());
    REQUIRE(content.find("define") != std::string::npos);
    REQUIRE(content.find("main") != std::string::npos);
    REQUIRE(content.find("ret") != std::string::npos);

    file.close();
  }
}

TEST_CASE("LLVMBackend type conversion", "[llvm_backend]") {
  SECTION("converts i32 type") {
    IRModule ir_module("test_types");
    auto func =
        std::make_unique<IRFunction>("main", IRType(IRType::Kind::I32));
    auto block = std::make_unique<IRBasicBlock>("entry");

    auto ret_instr =
        std::make_unique<IRInstruction>(IRInstruction::OpCode::RET);
    ret_instr->operands.push_back(
        IRValue::make_constant(IRType(IRType::Kind::I32), 1));

    block->instructions.push_back(std::move(ret_instr));
    func->basic_blocks.push_back(std::move(block));
    ir_module.functions.push_back(std::move(func));

    LLVMBackend backend;
    auto result = backend.generate(ir_module);

    REQUIRE(result.has_value());
  }
}

TEST_CASE("LLVMBackend handles multiple functions", "[llvm_backend]") {
  SECTION("multiple functions in one module") {
    IRModule ir_module("test_multi");

    // Function 1: foo returns 10
    auto func1 =
        std::make_unique<IRFunction>("foo", IRType(IRType::Kind::I32));
    auto block1 = std::make_unique<IRBasicBlock>("entry");
    auto ret1 = std::make_unique<IRInstruction>(IRInstruction::OpCode::RET);
    ret1->operands.push_back(
        IRValue::make_constant(IRType(IRType::Kind::I32), 10));
    block1->instructions.push_back(std::move(ret1));
    func1->basic_blocks.push_back(std::move(block1));
    ir_module.functions.push_back(std::move(func1));

    // Function 2: bar returns 20
    auto func2 =
        std::make_unique<IRFunction>("bar", IRType(IRType::Kind::I32));
    auto block2 = std::make_unique<IRBasicBlock>("entry");
    auto ret2 = std::make_unique<IRInstruction>(IRInstruction::OpCode::RET);
    ret2->operands.push_back(
        IRValue::make_constant(IRType(IRType::Kind::I32), 20));
    block2->instructions.push_back(std::move(ret2));
    func2->basic_blocks.push_back(std::move(block2));
    ir_module.functions.push_back(std::move(func2));

    LLVMBackend backend;
    auto result = backend.generate(ir_module);

    REQUIRE(result.has_value());
    REQUIRE_FALSE(backend.has_errors());
  }
}