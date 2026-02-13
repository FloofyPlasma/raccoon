#pragma once

#include "raccoon/AST.hpp"

// Base classes that provides default traversal behavior
// Subclasses can override specific visit methods they care about
class ASTTraverser : public ASTVisitor {
public:
  // Expressions
  void visit(IntegerLiteral& node) override {
    // Default: do nothing for literals
    (void)node;
  }

  void visit(BinaryExpr &node) override {
    node.left->accept(*this);
    node.right->accept(*this);
  }

  void visit(IdentifierExpr& node) override {
    (void)node;
  }

  // Statements
  void visit(BlockStmt& node) override {
    for (auto& stmt : node.statements) {
      stmt->accept(*this);
    }
  }

  void visit(ReturnStmt& node) override {
    if (node.value) {
      node.value->accept(*this);
    }
  }

  void visit(VarDeclStmt &node) override {
    if (node.initializer) {
      node.initializer->accept(*this);
    }
  }

  void visit(AssignmentStmt &node) override {
    node.value->accept(*this);
  }

  void visit(ExpressionStmt &node) override {
    node.expression->accept(*this);
  }

  // Declarations
  void visit(FunctionDecl& node) override {
    if (node.body) {
      node.body->accept(*this);
    }
  }

  void visit(ProgramNode& node) override {
    for (auto& func : node.functions) {
      func->accept(*this);
    }
  }
};