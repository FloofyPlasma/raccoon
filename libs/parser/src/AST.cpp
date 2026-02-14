#include "raccoon/AST.hpp"

// Accept method implementations for all AST nodes

void IntegerLiteral::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void BoolLiteral::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void BinaryExpr::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void UnaryExpr::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void IdentifierExpr::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void BlockStmt::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ReturnStmt::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void VarDeclStmt::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void AssignmentStmt::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ExpressionStmt::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void IfStmt::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void WhileStmt::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ForStmt::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void BreakStmt::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ContinueStmt::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void FunctionDecl::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ProgramNode::accept(ASTVisitor &visitor) { visitor.visit(*this); }
