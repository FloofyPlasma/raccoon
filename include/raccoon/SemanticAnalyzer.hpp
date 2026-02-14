#pragma once

#include "raccoon/AST.hpp"
#include "raccoon/ASTVisitor.hpp"
#include "raccoon/SymbolTable.hpp"
#include <expected>
#include <string>
#include <vector>

struct SemanticError {
  SourceLocation location;
  std::string message;
};

class SemanticAnalyzer : public ASTTraverser {
public:
  SemanticAnalyzer();

  std::expected<void, std::vector<SemanticError>> analyze(ProgramNode &program);

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
  std::unique_ptr<SymbolTable> global_scope;
  SymbolTable *current_scope;
  std::vector<SemanticError> errors;
  Type *current_function_return_type = nullptr;
  int loop_depth = 0;

  static constexpr std::size_t MAX_ERRORS = 20;

  void error(SourceLocation loc, const std::string &message);

  void push_scope();
  void pop_scope();

  Type *get_expression_type(Expression &expr);
  bool types_equal(const Type *a, const Type *b);

  bool verify_all_paths_return(BlockStmt &block, Type *return_type);
  std::string type_name(const Type* t);

  std::vector<std::unique_ptr<SymbolTable>> scope_storage;
  std::vector<SymbolTable *> scope_stack;
};
