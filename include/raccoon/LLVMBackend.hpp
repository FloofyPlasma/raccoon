#pragma once

#include "raccoon/IR.hpp"
#include "raccoon/Token.hpp"

#include <expected>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

struct LLVMError {
  SourceLocation location;
  std::string message;
};

class LLVMBackend {
public:
  LLVMBackend();
  ~LLVMBackend();

  std::expected<void, std::vector<LLVMError>>
  generate(const IRModule &ir_module);

  bool emit_object_file(const std::string &filename);

  bool emit_llvm_ir(const std::string &filename);

  bool has_errors() const { return !errors.empty(); }
  const std::vector<LLVMError> &get_errors() const { return errors; }

private:
  llvm::LLVMContext context;
  std::unique_ptr<llvm::Module> module;
  llvm::IRBuilder<> builder;

  std::vector<LLVMError> errors;

  std::unordered_map<std::string, llvm::Value *> value_map;
  std::unordered_map<std::string, llvm::BasicBlock *> label_map;

  void generate_function(const IRFunction &func);
  void generate_basic_block(const IRBasicBlock &block);
  void generate_instruction(const IRInstruction &instr);

  llvm::Type *convert_type(const IRType &ir_type);

  llvm::Value *get_value(const IRValue &ir_value);
  llvm::Constant *get_constant(const IRValue &ir_value);

  void error(const std::string &message);
};
