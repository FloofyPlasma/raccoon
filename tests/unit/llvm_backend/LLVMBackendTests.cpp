#include <catch2/catch_test_macros.hpp>

#include "raccoon/LLVMBackend.hpp"
#include <fstream>
#include <iostream>

std::unique_ptr<IRModule> make_ret_const(const std::string &name, int64_t val) {
  auto mod = std::make_unique<IRModule>(name);
  auto func = std::make_unique<IRFunction>("main", IRType(IRType::Kind::I32));
  auto block = std::make_unique<IRBasicBlock>("entry");

  auto ret = std::make_unique<IRInstruction>(IRInstruction::OpCode::RET);
  ret->operands.push_back(
      IRValue::make_constant(IRType(IRType::Kind::I32), val));
  block->instructions.push_back(std::move(ret));
  func->basic_blocks.push_back(std::move(block));
  mod->functions.push_back(std::move(func));
  return mod;
}

TEST_CASE("LLVMBackend generates empty module", "[llvm_backend]") {
  IRModule ir_module("test_empty");
  LLVMBackend backend;
  auto result = backend.generate(ir_module);
  REQUIRE(result.has_value());
}

TEST_CASE("LLVMBackend translates minimal function", "[llvm_backend]") {
  auto mod = make_ret_const("test_main", 42);
  LLVMBackend backend;
  auto result = backend.generate(*mod);
  REQUIRE(result.has_value());
  REQUIRE_FALSE(backend.has_errors());
}

TEST_CASE("LLVMBackend translates arithmetic instructions", "[llvm_backend]") {
  SECTION("add") {
    IRModule mod("test_add");
    auto func = std::make_unique<IRFunction>("main", IRType(IRType::Kind::I32));
    auto block = std::make_unique<IRBasicBlock>("entry");

    auto add = std::make_unique<IRInstruction>(IRInstruction::OpCode::ADD);
    add->result = IRValue::make_register(IRType(IRType::Kind::I32), "sum");
    add->operands.push_back(
        IRValue::make_constant(IRType(IRType::Kind::I32), 10));
    add->operands.push_back(
        IRValue::make_constant(IRType(IRType::Kind::I32), 20));
    block->instructions.push_back(std::move(add));

    auto ret = std::make_unique<IRInstruction>(IRInstruction::OpCode::RET);
    ret->operands.push_back(
        IRValue::make_register(IRType(IRType::Kind::I32), "sum"));
    block->instructions.push_back(std::move(ret));

    func->basic_blocks.push_back(std::move(block));
    mod.functions.push_back(std::move(func));

    LLVMBackend backend;
    auto result = backend.generate(mod);
    REQUIRE(result.has_value());
  }

  SECTION("sub, mul, sdiv") {
    for (auto op : {IRInstruction::OpCode::SUB, IRInstruction::OpCode::MUL,
                    IRInstruction::OpCode::SDIV}) {
      IRModule mod("test_arith");
      auto func =
          std::make_unique<IRFunction>("main", IRType(IRType::Kind::I32));
      auto block = std::make_unique<IRBasicBlock>("entry");

      auto instr = std::make_unique<IRInstruction>(op);
      instr->result = IRValue::make_register(IRType(IRType::Kind::I32), "r");
      instr->operands.push_back(
          IRValue::make_constant(IRType(IRType::Kind::I32), 10));
      instr->operands.push_back(
          IRValue::make_constant(IRType(IRType::Kind::I32), 3));
      block->instructions.push_back(std::move(instr));

      auto ret = std::make_unique<IRInstruction>(IRInstruction::OpCode::RET);
      ret->operands.push_back(
          IRValue::make_register(IRType(IRType::Kind::I32), "r"));
      block->instructions.push_back(std::move(ret));

      func->basic_blocks.push_back(std::move(block));
      mod.functions.push_back(std::move(func));

      LLVMBackend backend;
      auto result = backend.generate(mod);
      REQUIRE(result.has_value());
    }
  }
}

TEST_CASE("LLVMBackend translates alloca/store/load", "[llvm_backend]") {
  IRModule mod("test_var");
  auto func = std::make_unique<IRFunction>("main", IRType(IRType::Kind::I32));
  auto block = std::make_unique<IRBasicBlock>("entry");

  // %ptr = alloca i32
  auto alloca = std::make_unique<IRInstruction>(IRInstruction::OpCode::ALLOCA);
  alloca->result = IRValue::make_register(IRType(IRType::Kind::I32), "ptr");
  block->instructions.push_back(std::move(alloca));

  // store i32 42, i32* %ptr
  auto store = std::make_unique<IRInstruction>(IRInstruction::OpCode::STORE);
  store->operands.push_back(
      IRValue::make_constant(IRType(IRType::Kind::I32), 42));
  store->operands.push_back(
      IRValue::make_register(IRType(IRType::Kind::I32), "ptr"));
  block->instructions.push_back(std::move(store));

  // %val = load i32, i32* %ptr
  auto load = std::make_unique<IRInstruction>(IRInstruction::OpCode::LOAD);
  load->result = IRValue::make_register(IRType(IRType::Kind::I32), "val");
  load->operands.push_back(
      IRValue::make_register(IRType(IRType::Kind::I32), "ptr"));
  block->instructions.push_back(std::move(load));

  // ret i32 %val
  auto ret = std::make_unique<IRInstruction>(IRInstruction::OpCode::RET);
  ret->operands.push_back(
      IRValue::make_register(IRType(IRType::Kind::I32), "val"));
  block->instructions.push_back(std::move(ret));

  func->basic_blocks.push_back(std::move(block));
  mod.functions.push_back(std::move(func));

  LLVMBackend backend;
  auto result = backend.generate(mod);

  if (!result.has_value()) {
    for (const auto &err : result.error()) {
      std::cerr << "Error: " << err.message << "\n";
    }
  }

  REQUIRE(result.has_value());
}

TEST_CASE("LLVMBackend handles errors", "[llvm_backend]") {
  SECTION("missing terminator fails verification") {
    IRModule ir_module("test_invalid");
    auto func = std::make_unique<IRFunction>("main", IRType(IRType::Kind::I32));
    auto block = std::make_unique<IRBasicBlock>("entry");
    func->basic_blocks.push_back(std::move(block));
    ir_module.functions.push_back(std::move(func));

    LLVMBackend backend;
    auto result = backend.generate(ir_module);
    REQUIRE_FALSE(result.has_value());
    REQUIRE(backend.has_errors());
  }
}

TEST_CASE("LLVMBackend emits object file", "[llvm_backend]") {
  auto mod = make_ret_const("test_emit", 42);
  LLVMBackend backend;
  auto gen_result = backend.generate(*mod);
  REQUIRE(gen_result.has_value());

  bool success = backend.emit_object_file("test_emit.o");
  REQUIRE(success);

  std::ifstream file("test_emit.o", std::ios::binary | std::ios::ate);
  REQUIRE(file.is_open());
  REQUIRE(file.tellg() > 0);
  file.close();
}

TEST_CASE("LLVMBackend emits LLVM IR text", "[llvm_backend]") {
  auto mod = make_ret_const("test_ir", 99);
  LLVMBackend backend;
  auto gen_result = backend.generate(*mod);
  REQUIRE(gen_result.has_value());

  bool success = backend.emit_llvm_ir("test_ir.ll");
  REQUIRE(success);

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
