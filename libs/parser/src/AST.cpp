#include "raccoon/AST.hpp"

// Accept method implementations for all AST nodes

void IntegerLiteral::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void BinaryExpr::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void IdentifierExpr::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void BlockStmt::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void ReturnStmt::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void VarDeclStmt::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void AssignmentStmt::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void ExpressionStmt::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void FunctionDecl::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void ProgramNode::accept(ASTVisitor &visitor) { visitor.visit(*this); }
