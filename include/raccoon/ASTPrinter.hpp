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
    if (!type) {
      return "unknown";
    }

    switch (type->kind) {
    case Type::Kind::I32:
      return "i32";

    case Type::Kind::BOOL:
      return "bool";

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
    case TokenType::LESS:
      return "<";
    case TokenType::GREATER:
      return ">";
    case TokenType::LESS_EQUAL:
      return "<=";
    case TokenType::GREATER_EQUAL:
      return ">=";
    case TokenType::EQUAL_EQUAL:
      return "==";
    case TokenType::BANG_EQUAL:
      return "!=";
    case TokenType::AND_AND:
      return "&&";
    case TokenType::OR_OR:
      return "||";
    case TokenType::BANG:
      return "!";
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

  void visit(IfStmt &node) override {
    print_indent();
    std::println("IfStmt");
    indent++;
    print_indent();
    std::println("Condition:");
    indent++;
    node.condition->accept(*this);
    indent--;
    print_indent();
    std::println("Then:");
    node.then_branch->accept(*this);
    if (node.else_branch) {
      print_indent();
      std::println("Else:");
      node.else_branch->accept(*this);
    }
    indent--;
  }

  void visit(WhileStmt &node) override {
    print_indent();
    std::println("WhileStmt");
    indent++;
    print_indent();
    std::println("Condition:");
    indent++;
    node.condition->accept(*this);
    indent--;
    print_indent();
    std::println("Body:");
    node.body->accept(*this);
    indent--;
  }

  void visit(ForStmt &node) override {
    print_indent();
    std::println("ForStmt");
    indent++;
    if (node.initializer) {
      print_indent();
      std::println("Init:");
      indent++;
      node.initializer->accept(*this);
      indent--;
    }
    if (node.condition) {
      print_indent();
      std::println("Condition:");
      indent++;
      node.condition->accept(*this);
      indent--;
    }
    if (node.increment) {
      print_indent();
      std::println("Increment:");
      indent++;
      node.increment->accept(*this);
      indent--;
    }
    print_indent();
    std::println("Body:");
    node.body->accept(*this);
    indent--;
  }

  void visit(BreakStmt &node) override {
    (void)node;
    print_indent();
    std::println("BreakStmt");
  }

  void visit(ContinueStmt &node) override {
    (void)node;
    print_indent();
    std::println("ContinueStmt");
  }

  void visit(BinaryExpr &node) override {
    print_indent();
    std::println("BinaryExpr: {}", op_to_string(node.op));
    indent++;
    node.left->accept(*this);
    node.right->accept(*this);
    indent--;
  }

  void visit(UnaryExpr &node) override {
    print_indent();
    std::println("UnaryExpr: {}", op_to_string(node.op));
    indent++;
    node.operand->accept(*this);
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

  void visit(BoolLiteral &node) override {
    print_indent();
    std::println("BoolLiteral: {}", node.value ? "true" : "false");
  }
};