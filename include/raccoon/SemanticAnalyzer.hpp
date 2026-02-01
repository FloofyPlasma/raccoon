#pragma once

#include "raccoon/AST.hpp"
#include "raccoon/SymbolTable.hpp"
#include "raccoon/ASTVisitor.hpp"
#include <expected>
#include <vector>
#include <string>

struct SemanticError {
  SourceLocation location;
  std::string message;
};

class SemanticAnalyzer : public ASTTraverser {
public:
  SemanticAnalyzer();

  std::expected<void, std::vector<SemanticError>> analyze(ProgramNode& program);

  void visit(ProgramNode& node) override;
  void visit(FunctionDecl& node) override;
  void visit(ReturnStmt& node) override;
  void visit(IntegerLiteral& node) override;

private:
  std::unique_ptr<SymbolTable> global_scope;
  SymbolTable* current_scope;
  std::vector<SemanticError> errors;
  Type* current_function_return_type = nullptr;

  static constexpr std::size_t MAX_ERRORS = 20;

  void error(SourceLocation loc, const std::string&message);

  Type* get_expression_type(Expression& expr);
  bool types_equal(const Type* a, const Type* b);

  bool verify_all_paths_return(BlockStmt& block, Type* return_type);
};
