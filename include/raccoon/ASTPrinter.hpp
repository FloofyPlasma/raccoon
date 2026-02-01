#pragma once

#include "raccoon/ASTVisitor.hpp"
#include <print>

class ASTPrinter : public ASTTraverser {
  int indent = 0;

  void print_indent() {
    for (int i = 0; i < indent; i++ ) {
      std::print("  ");
    }
  }

public:
  void visit(ProgramNode& node) override {
    std::println("Program");
    indent++;
    ASTTraverser::visit(node);
    indent--;
  }

  void visit(FunctionDecl& node) override {
    print_indent();
    std::println("FunctionDecl: {}", node.name);
    indent++;

    print_indent();
    std::print("ReturnType: ");
    switch (node.return_type->kind) {
    case Type::Kind::I32:
      std::println("i32");
      break;
    case Type::Kind::VOID:
      std::println("void");
      break;
    }

    if (node.body) {
      node.body->accept(*this);
    }
    indent--;
  }

  void visit(BlockStmt& node) override {
    print_indent();
    std::println("Block");
    indent++;
    ASTTraverser::visit(node);
    indent--;
  }

  void visit(ReturnStmt& node) override {
    print_indent();
    std::println("ReturnStmt");
    indent++;
    ASTTraverser::visit(node);
    indent--;
  }

  void visit(IntegerLiteral& node) override {
    print_indent();
    std::println("IntegerLiteral: {}", node.value);
  }
};