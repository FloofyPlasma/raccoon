#pragma once

#include "raccoon/Token.hpp"
#include <memory>
#include <string>
#include <vector>

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
    BOOL,
    VOID,
  };

  Kind kind;

  explicit Type(Kind k, SourceLocation loc = {}) : ASTNode(loc), kind(k) {}
};

#pragma mark Expressions

class Expression : public ASTNode {
public:
  enum class Kind {
    INTEGER_LITERAL,
    BOOL_LITERAL,
    BINARY_EXPR,
    UNARY_EXPR,
    IDENTIFIER,
  };

  Kind kind;

  std::unique_ptr<Type> resolved_type;

  virtual void accept(ASTVisitor &visitor) = 0;

protected:
  Expression(Kind k, SourceLocation loc) : ASTNode(loc), kind(k) {}
};

class IntegerLiteral : public Expression {
public:
  int64_t value;

  IntegerLiteral(int64_t val, SourceLocation loc)
      : Expression(Kind::INTEGER_LITERAL, loc), value(val) {}

  void accept(ASTVisitor &visitor) override;
};

class BinaryExpr : public Expression {
public:
  std::unique_ptr<Expression> left;
  TokenType op;
  std::unique_ptr<Expression> right;

  BinaryExpr(std::unique_ptr<Expression> l, TokenType o,
             std::unique_ptr<Expression> r, SourceLocation loc)
      : Expression(Kind::BINARY_EXPR, loc), left(std::move(l)), op(o),
        right(std::move(r)) {}

  void accept(ASTVisitor &visitor) override;
};

class IdentifierExpr : public Expression {
public:
  std::string name;

  IdentifierExpr(std::string n, SourceLocation loc)
      : Expression(Kind::IDENTIFIER, loc), name(std::move(n)) {}

  void accept(ASTVisitor &visitor) override;
};

class BoolLiteral : public Expression {
public:
  bool value;

  BoolLiteral(bool val, SourceLocation loc)
      : Expression(Kind::BOOL_LITERAL, loc), value(val) {}

  void accept(ASTVisitor &visitor) override;
};

class UnaryExpr : public Expression {
public:
  TokenType op; // MINUS, BANG
  std::unique_ptr<Expression> operand;

  UnaryExpr(TokenType o, std::unique_ptr<Expression> operand,
            SourceLocation loc)
      : Expression(Kind::UNARY_EXPR, loc), op(o), operand(std::move(operand)) {}

  void accept(ASTVisitor &visitor) override;
};

#pragma mark Statements

class Statement : public ASTNode {
public:
  enum class Kind {
    BLOCK,
    RETURN,
    VAR_DECL,
    ASSIGNMENT,
    EXPRESSION,
    IF,
    WHILE,
    FOR,
    BREAK,
    CONTINUE,
  };

  Kind kind;

  virtual void accept(ASTVisitor &visitor) = 0;

protected:
  Statement(Kind k, SourceLocation loc) : ASTNode(loc), kind(k) {}
};

class BlockStmt : public Statement {
public:
  std::vector<std::unique_ptr<Statement>> statements;

  explicit BlockStmt(SourceLocation loc) : Statement(Kind::BLOCK, loc) {}

  void accept(ASTVisitor &visitor) override;
};

class ReturnStmt : public Statement {
public:
  std::unique_ptr<Expression> value;

  ReturnStmt(std::unique_ptr<Expression> val, SourceLocation loc)
      : Statement(Kind::RETURN, loc), value(std::move(val)) {}

  void accept(ASTVisitor &visitor) override;
};

class VarDeclStmt : public Statement {
public:
  std::string name;
  std::unique_ptr<Type> type;
  std::unique_ptr<Expression>
      initializer; // may be nullptr for uninitialized data

  VarDeclStmt(std::string name, std::unique_ptr<Type> type,
              std::unique_ptr<Expression> init, SourceLocation loc)
      : Statement(Kind::VAR_DECL, loc), name(std::move(name)),
        type(std::move(type)), initializer(std::move(init)) {}

  void accept(ASTVisitor &visitor) override;
};

class AssignmentStmt : public Statement {
public:
  std::string name;
  std::unique_ptr<Expression> value;

  AssignmentStmt(std::string name, std::unique_ptr<Expression> val,
                 SourceLocation loc)
      : Statement(Kind::ASSIGNMENT, loc), name(std::move(name)),
        value(std::move(val)) {}

  void accept(ASTVisitor &visitor) override;
};

class ExpressionStmt : public Statement {
public:
  std::unique_ptr<Expression> expression;

  ExpressionStmt(std::unique_ptr<Expression> expr, SourceLocation loc)
      : Statement(Kind::EXPRESSION, loc), expression(std::move(expr)) {}

  void accept(ASTVisitor &visitor) override;
};

class IfStmt : public Statement {
public:
  std::unique_ptr<Expression> condition;
  std::unique_ptr<BlockStmt> then_branch;
  std::unique_ptr<BlockStmt> else_branch; // nullptr if no else

  IfStmt(std::unique_ptr<Expression> cond, std::unique_ptr<BlockStmt> then_b,
         std::unique_ptr<BlockStmt> else_b, SourceLocation loc)
      : Statement(Kind::IF, loc), condition(std::move(cond)),
        then_branch(std::move(then_b)), else_branch(std::move(else_b)) {}

  void accept(ASTVisitor &visitor) override;
};

class WhileStmt : public Statement {
public:
  std::unique_ptr<Expression> condition;
  std::unique_ptr<BlockStmt> body;

  WhileStmt(std::unique_ptr<Expression> cond, std::unique_ptr<BlockStmt> body,
            SourceLocation loc)
      : Statement(Kind::WHILE, loc), condition(std::move(cond)),
        body(std::move(body)) {}

  void accept(ASTVisitor &visitor) override;
};

class ForStmt : public Statement {
public:
  std::unique_ptr<Statement>
      initializer; // VarDeclStmt or ExpressionStmt, or nullptr
  std::unique_ptr<Expression> condition;
  std::unique_ptr<Statement>
      increment; // AssignmentStmt or ExpressionStmt, or nullptr
  std::unique_ptr<BlockStmt> body;

  ForStmt(std::unique_ptr<Statement> init, std::unique_ptr<Expression> cond,
          std::unique_ptr<Statement> incr, std::unique_ptr<BlockStmt> body,
          SourceLocation loc)
      : Statement(Kind::FOR, loc), initializer(std::move(init)),
        condition(std::move(cond)), increment(std::move(incr)),
        body(std::move(body)) {}

  void accept(ASTVisitor &visitor) override;
};

class BreakStmt : public Statement {
public:
  explicit BreakStmt(SourceLocation loc) : Statement(Kind::BREAK, loc) {}

  void accept(ASTVisitor &visitor) override;
};

class ContinueStmt : public Statement {
public:
  explicit ContinueStmt(SourceLocation loc) : Statement(Kind::CONTINUE, loc) {}

  void accept(ASTVisitor &visitor) override;
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

  FunctionDecl(std::string name, SourceLocation loc)
      : ASTNode(loc), name(std::move(name)) {}

  void accept(ASTVisitor &visitor);
};

class ProgramNode : public ASTNode {
public:
  std::vector<std::unique_ptr<FunctionDecl>> functions;

  ProgramNode() = default;

  void accept(ASTVisitor &visitor);
};

#pragma mark Visitor Interface

class ASTVisitor {
public:
  virtual ~ASTVisitor() = default;

  // Expressions
  virtual void visit(IntegerLiteral &node) = 0;
  virtual void visit(BoolLiteral &node) = 0;
  virtual void visit(BinaryExpr &node) = 0;
  virtual void visit(UnaryExpr &node) = 0;
  virtual void visit(IdentifierExpr &node) = 0;

  // Statements
  virtual void visit(BlockStmt &node) = 0;
  virtual void visit(ReturnStmt &node) = 0;
  virtual void visit(VarDeclStmt &node) = 0;
  virtual void visit(AssignmentStmt &node) = 0;
  virtual void visit(ExpressionStmt &node) = 0;
  virtual void visit(IfStmt &node) = 0;
  virtual void visit(WhileStmt &node) = 0;
  virtual void visit(ForStmt &node) = 0;
  virtual void visit(BreakStmt &node) = 0;
  virtual void visit(ContinueStmt &node) = 0;

  // Declarations
  virtual void visit(FunctionDecl &node) = 0;
  virtual void visit(ProgramNode &node) = 0;
};
