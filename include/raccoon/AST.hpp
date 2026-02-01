#pragma once

#include "raccoon/Token.hpp"
#include <memory>
#include <vector>
#include <string>

class Type;
class Expression;
class Statement;
class ASTVisitor;

#pragma mark Base Nodes

class ASTNode {
public:
  SourceLocation location;
  virtual ~ASTNode() = default;

protected:
  ASTNode() = default;
  explicit ASTNode(SourceLocation loc) : location(loc) {}
};

#pragma mark Types

class Type : public ASTNode {
public:
  enum class Kind {
    I32,
    VOID,
  };

  Kind kind;

  explicit Type(Kind k, SourceLocation loc = {})
        : ASTNode(loc), kind(k) {}
};

#pragma mark Expressions

class Expression : public ASTNode {
public:
  enum class Kind {
    INTEGER_LITERAL,
  };

  Kind kind;

  std::unique_ptr<Type> resolved_type;

  virtual void accept(ASTVisitor& visitor) = 0;

protected:
  Expression(Kind k, SourceLocation loc) : ASTNode(loc), kind(k) {}
};

class IntegerLiteral : public Expression {
public:
  int64_t value;

  IntegerLiteral(int64_t val, SourceLocation loc) : Expression(Kind::INTEGER_LITERAL, loc), value(val) {}

  void accept(ASTVisitor& visitor) override;
};

#pragma mark Statements

class Statement : public ASTNode {
public:
  enum class Kind {
    BLOCK,
    RETURN
  };

  Kind kind;

  virtual void accept(ASTVisitor& visitor) = 0;

protected:
  Statement(Kind k, SourceLocation loc) : ASTNode(loc), kind(k) {}
};

class BlockStmt : public Statement {
public:
  std::vector<std::unique_ptr<Statement>> statements;

  explicit BlockStmt(SourceLocation loc)
      : Statement(Kind::BLOCK, loc) {}

  void accept(ASTVisitor& visitor) override;
};

class ReturnStmt : public Statement {
public:
  std::unique_ptr<Expression> value;

  ReturnStmt(std::unique_ptr<Expression> val, SourceLocation loc)
      : Statement(Kind::RETURN, loc), value(std::move(val)) {}

  void accept(ASTVisitor& visitor) override;
};

#pragma mark Declarations

struct Parameter {
  std::string name;
  std::unique_ptr<Type> type;
  SourceLocation loc;
};

class FunctionDecl : public ASTNode {
public:
  std::string name;
  std::vector<Parameter> parameters;
  std::unique_ptr<Type> return_type;
  std::unique_ptr<BlockStmt> body;

  FunctionDecl(std::string name, SourceLocation loc) : ASTNode(loc), name(std::move(name)) {}

  void accept(ASTVisitor& visitor);
};

class ProgramNode : public ASTNode {
public:
  std::vector<std::unique_ptr<FunctionDecl>> functions;

  ProgramNode() = default;

  void accept(ASTVisitor& visitor);
};

#pragma mark Visitor Interface

class ASTVisitor {
public:
  virtual ~ASTVisitor() = default;

  // Expressions
  virtual void visit(IntegerLiteral& node) = 0;

  // Statements
  virtual void visit(BlockStmt& node) = 0;
  virtual void visit(ReturnStmt& node) = 0;

  // Declarations
  virtual void visit(FunctionDecl& node) = 0;
  virtual void visit(ProgramNode& node) = 0;
};
