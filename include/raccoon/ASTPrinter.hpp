#pragma once

#include "raccoon/ASTVisitor.hpp"
#include <print>

class ASTPrinter : public ASTTraverser {
  int indent = 0;

  void print_indent() {
    for (int i = 0; i < indent; i++) {
      std::print("  ");
    }
  }

  std::string type_to_string(const Type *type) {
    if (!type)
      return "unknown";
    switch (type->kind) {

    case Type::Kind::I32:
      return "i32";

    case Type::Kind::VOID:
      return "void";
    }
    return "unknown";
  }

  std::string op_to_string(TokenType op) {
    switch (op) {
    case TokenType::PLUS:
      return "+";
    case TokenType::MINUS:
      return "-";
    case TokenType::STAR:
      return "*";
    case TokenType::SLASH:
      return "/";
    default:
      return "?";
    }
  }

public:
  void visit(ProgramNode &node) override {
    std::println("Program");
    indent++;
    ASTTraverser::visit(node);
    indent--;
  }

  void visit(FunctionDecl &node) override {
    print_indent();
    std::println("FunctionDecl: {}", node.name);
    indent++;

    print_indent();
    std::println("ReturnType: {}", type_to_string(node.return_type.get()));

    if (node.body) {
      node.body->accept(*this);
    }
    indent--;
  }

  void visit(BlockStmt &node) override {
    print_indent();
    std::println("Block");
    indent++;
    ASTTraverser::visit(node);
    indent--;
  }

  void visit(ReturnStmt &node) override {
    print_indent();
    std::println("ReturnStmt");
    indent++;
    ASTTraverser::visit(node);
    indent--;
  }

  void visit(VarDeclStmt &node) override {
    print_indent();
    std::println("VarDecl: {} : {}", node.name,
                 type_to_string(node.type.get()));
    if (node.initializer) {
      indent++;
      node.initializer->accept(*this);
      indent--;
    }
  }

  void visit(AssignmentStmt &node) override {
    print_indent();
    std::println("Assignment: {}", node.name);
    indent++;
    node.value->accept(*this);
    indent--;
  }

  void visit(ExpressionStmt &node) override {
    print_indent();
    std::println("ExpressionStmt");
    indent++;
    node.expression->accept(*this);
    indent--;
  }

  void visit(BinaryExpr &node) override {
    print_indent();
    std::println("BinaryExpr: {}", op_to_string(node.op));
    indent++;
    node.left->accept(*this);
    node.right->accept(*this);
    indent--;
  }

  void visit(IdentifierExpr &node) override {
    print_indent();
    std::println("Identifier: {}", node.name);
  }

  void visit(IntegerLiteral &node) override {
    print_indent();
    std::println("IntegerLiteral: {}", node.value);
  }
};