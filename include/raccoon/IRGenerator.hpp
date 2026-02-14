#pragma once

#include "raccoon/AST.hpp"
#include "raccoon/ASTVisitor.hpp"
#include "raccoon/IR.hpp"
#include <memory>
#include <unordered_map>

class IRGenerator : public ASTTraverser {
public:
  IRGenerator();

  std::unique_ptr<IRModule> generate(ProgramNode &program,
                                     const std::string module_name);

  // Visitor overrides
  void visit(ProgramNode &node) override;
  void visit(FunctionDecl &node) override;
  void visit(BlockStmt &node) override;
  void visit(ReturnStmt &node) override;
  void visit(IntegerLiteral &node) override;
  void visit(BoolLiteral &node) override;
  void visit(BinaryExpr &node) override;
  void visit(UnaryExpr &node) override;
  void visit(IdentifierExpr &node) override;
  void visit(VarDeclStmt &node) override;
  void visit(AssignmentStmt &node) override;
  void visit(ExpressionStmt &node) override;
  void visit(IfStmt &node) override;
  void visit(WhileStmt &node) override;
  void visit(ForStmt &node) override;
  void visit(BreakStmt &node) override;
  void visit(ContinueStmt &node) override;

private:
  std::unique_ptr<IRModule> module;
  IRFunction *current_function = nullptr;
  IRBasicBlock *current_block = nullptr;

  int next_register = 0;
  int next_label = 0;

  IRValue last_value;

  std::unordered_map<std::string, IRValue> variables;

  struct LoopContext {
    std::string continue_label;
    std::string break_label;
  };
  std::vector<LoopContext> loop_stack;

  IRValue new_register(IRType type);
  std::string new_label(const std::string &prefix);
  IRType convert_type(const Type *ast_type);
  void emit(std::unique_ptr<IRInstruction> instr);
  bool current_block_has_terminator() const;
  void switch_to_block(const std::string &label);
};