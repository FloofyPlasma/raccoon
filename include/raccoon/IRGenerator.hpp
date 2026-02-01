#pragma once

#include <memory>
#include "raccoon/AST.hpp"
#include "raccoon/IR.hpp"
#include "raccoon/ASTVisitor.hpp"
#include <unordered_map>

class IRGenerator : public ASTTraverser {
public:
  IRGenerator();

  std::unique_ptr<IRModule> generate(ProgramNode& program, const std::string module_name);

  // Visitor overrides
  void visit(ProgramNode& node) override;
  void visit(FunctionDecl& node) override;
  void visit(BlockStmt& node) override;
  void visit(ReturnStmt& node) override;
  void visit(IntegerLiteral& node) override;

private:
  std::unique_ptr<IRModule> module;
  IRFunction* current_function = nullptr;
  IRBasicBlock* current_block = nullptr;

  int next_register = 0;
  int next_label = 0;

  IRValue last_value;

  IRValue new_register(IRType type);
  std::string new_label(const std::string& prefix);
  IRType convert_type(const Type* ast_type);
  void emit(std::unique_ptr<IRInstruction> instr);
};