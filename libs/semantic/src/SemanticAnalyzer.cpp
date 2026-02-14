#include "raccoon/SemanticAnalyzer.hpp"
#include <cassert>
#include <print>

SemanticAnalyzer::SemanticAnalyzer()
    : global_scope(std::make_unique<SymbolTable>()),
      current_scope(global_scope.get()) {}

std::expected<void, std::vector<SemanticError>>
SemanticAnalyzer::analyze(ProgramNode &program) {
  errors.clear();
  scope_storage.clear();
  scope_stack.clear();
  current_scope = global_scope.get();
  loop_depth = 0;

  visit(program);

  if (!errors.empty()) {
    return std::unexpected(std::move(errors));
  }

  return {};
}

std::string SemanticAnalyzer::type_name(const Type *t) {
  if (!t) {
    return "unknown";
  }
  switch (t->kind) {
  case Type::Kind::I32:
    return "i32";
  case Type::Kind::BOOL:
    return "bool";
  case Type::Kind::VOID:
    return "void";
  }

  return "unknown";
}

void SemanticAnalyzer::visit(ProgramNode &node) {
  // First pass: Collect all function declarations
  for (auto &func : node.functions) {
    // Check for duplicate function names
    if (current_scope->lookup_local(func->name)) {
      error(func->location, "Function '" + func->name + "' is already defined");
      continue;
    }

    auto symbol = std::make_unique<Symbol>();
    symbol->name = func->name;
    symbol->kind = SymbolKind::FUNCTION;
    symbol->type = func->return_type.get();
    symbol->location = func->location;

    current_scope->insert(func->name, std::move(symbol));
  }

  // Second pass: Type check function bodies
  for (auto &func : node.functions) {
    func->accept(*this);

    if (errors.size() >= MAX_ERRORS) {
      break;
    }
  }
}

void SemanticAnalyzer::visit(FunctionDecl &node) {
  current_function_return_type = node.return_type.get();

  push_scope();

  if (node.body) {
    node.body->accept(*this);

    // Verify all paths return
    if (!verify_all_paths_return(*node.body, node.return_type.get())) {
      error(node.location, "Expected return statement in non-void function '" +
                               node.name + "'");
    }
  }

  pop_scope();
  current_function_return_type = nullptr;
}

void SemanticAnalyzer::visit(BlockStmt &node) {
  push_scope();
  for (auto &stmt : node.statements) {
    stmt->accept(*this);
  }
  pop_scope();
}

void SemanticAnalyzer::visit(ReturnStmt &node) {
  if (node.value) {
    node.value->accept(*this);

    Type *value_type = get_expression_type(*node.value);

    if (value_type && current_function_return_type) {
      if (!types_equal(value_type, current_function_return_type)) {
        error(node.location, "Cannot return value of type '" +
                                 type_name(value_type) +
                                 "' from function with return type '" +
                                 type_name(current_function_return_type) + "'");
      }
    }
  } else {
    if (current_function_return_type &&
        current_function_return_type->kind != Type::Kind::VOID) {
      error(node.location,
            "Cannot return without value from non-void function");
    }
  }
}

void SemanticAnalyzer::visit(IntegerLiteral &node) {
  node.resolved_type = std::make_unique<Type>(Type::Kind::I32, node.location);
}

void SemanticAnalyzer::visit(BoolLiteral &node) {
  node.resolved_type = std::make_unique<Type>(Type::Kind::BOOL, node.location);
}

void SemanticAnalyzer::visit(BinaryExpr &node) {
  node.left->accept(*this);
  node.right->accept(*this);

  Type *left_type = get_expression_type(*node.left);
  Type *right_type = get_expression_type(*node.right);
  if (!left_type || !right_type) {
    return;
  }

  if (!types_equal(left_type, right_type)) {
    error(node.location, "Type mismatch in binary expression: '" +
                             type_name(left_type) + "' vs '" +
                             type_name(right_type) + "'");
    return;
  }

  switch (node.op) {
    // Arithmetic: i32 -> i32
  case TokenType::PLUS:
  case TokenType::MINUS:
  case TokenType::STAR:
  case TokenType::SLASH:
    if (left_type->kind != Type::Kind::I32) {
      error(node.location, "Arithmetic operators require 'i32' operands");
      return;
    }
    node.resolved_type = std::make_unique<Type>(Type::Kind::I32, node.location);
    break;

    // Comparison: i32 -> bool
  case TokenType::LESS:
  case TokenType::GREATER:
  case TokenType::LESS_EQUAL:
  case TokenType::GREATER_EQUAL:
    if (left_type->kind != Type::Kind::I32) {
      error(node.location, "Comparison operators require 'i32' operands");
      return;
    }
    node.resolved_type =
        std::make_unique<Type>(Type::Kind::BOOL, node.location);
    break;

  // Equality: same type -> bool
  case TokenType::EQUAL_EQUAL:
  case TokenType::BANG_EQUAL:
    node.resolved_type =
        std::make_unique<Type>(Type::Kind::BOOL, node.location);
    break;

    // Logical: bool -> bool
  case TokenType::AND_AND:
  case TokenType::OR_OR:
    if (left_type->kind != Type::Kind::BOOL) {
      error(node.location, "Logical operators require 'bool' operands");
      return;
    }
    node.resolved_type =
        std::make_unique<Type>(Type::Kind::BOOL, node.location);
    break;

  default:
    error(node.location, "Unknown binary operator");
    break;
  }
}

void SemanticAnalyzer::visit(UnaryExpr &node) {
  node.operand->accept(*this);

  Type *operand_type = get_expression_type(*node.operand);
  if (!operand_type) {
    return;
  }

  switch (node.op) {
  case TokenType::MINUS:
    if (operand_type->kind != Type::Kind::I32) {
      error(node.location, "Unary '-' requires 'i32' operand");
      return;
    }
    node.resolved_type = std::make_unique<Type>(Type::Kind::I32, node.location);
    break;

  case TokenType::BANG:
    if (operand_type->kind != Type::Kind::BOOL) {
      error(node.location, "Unary '!' requires 'bool' operand");
      return;
    }
    node.resolved_type =
        std::make_unique<Type>(Type::Kind::BOOL, node.location);
    break;

  default:
    error(node.location, "Unknown unary operator");
    break;
  }
}

void SemanticAnalyzer::visit(IdentifierExpr &node) {
  Symbol *sym = current_scope->lookup(node.name);
  if (!sym) {
    error(node.location, "Undefined variable '" + node.name + "'");
    return;
  }

  if (sym->type) {
    node.resolved_type = std::make_unique<Type>(sym->type->kind, node.location);
  }
}

void SemanticAnalyzer::visit(VarDeclStmt &node) {
  if (current_scope->lookup_local(node.name)) {
    error(node.location,
          "Variable '" + node.name + "' is already defined in this scope");
    return;
  }

  if (node.initializer) {
    node.initializer->accept(*this);

    Type *init_type = get_expression_type(*node.initializer);
    if (init_type && node.type) {
      if (!types_equal(init_type, node.type.get())) {
        error(node.location,
              "Type mismatch in variable initialization: expected '" +
                  type_name(node.type.get()) + "', got '" +
                  type_name(init_type) + "'");
      }
    }
  }

  auto symbol = std::make_unique<Symbol>();
  symbol->name = node.name;
  symbol->kind = SymbolKind::VARIABLE;
  symbol->type = node.type.get();
  symbol->location = node.location;

  current_scope->insert(node.name, std::move(symbol));
}

void SemanticAnalyzer::visit(AssignmentStmt &node) {
  Symbol *sym = current_scope->lookup(node.name);
  if (!sym) {
    error(node.location, "Undefined variable '" + node.name + "'");
    return;
  }

  node.value->accept(*this);

  Type *value_type = get_expression_type(*node.value);
  if (value_type && sym->type) {
    if (!types_equal(value_type, sym->type)) {
      error(node.location,
            "Type mismatch in assignment to '" + node.name + "'");
    }
  }
}

void SemanticAnalyzer::visit(ExpressionStmt &node) {
  node.expression->accept(*this);
}

void SemanticAnalyzer::visit(IfStmt &node) {
  node.condition->accept(*this);

  Type *cond_type = get_expression_type(*node.condition);
  if (cond_type && cond_type->kind != Type::Kind::BOOL) {
    error(node.location,
          "If condition must be 'bool', got '" + type_name(cond_type) + "'");
  }

  node.then_branch->accept(*this);
  if (node.else_branch) {
    node.else_branch->accept(*this);
  }
}

void SemanticAnalyzer::visit(WhileStmt &node) {
  node.condition->accept(*this);

  Type *cond_type = get_expression_type(*node.condition);
  if (cond_type && cond_type->kind != Type::Kind::BOOL) {
    error(node.location, "While condition must be 'bool', got '" + type_name(cond_type) + "'");
  }
  loop_depth++;
  node.body->accept(*this);
  loop_depth--;
}

void SemanticAnalyzer::visit(ForStmt &node) {
  push_scope();
  if (node.initializer) {node.initializer->accept(*this);}
  if (node.condition) {
    node.condition->accept(*this);

    Type* cond_type = get_expression_type(*node.condition);
    if (cond_type && cond_type->kind != Type::Kind::BOOL) {
      error(node.location, "For condition must be 'bool', got '" + type_name(cond_type) + "'");
    }
  }
  if (node.increment) {node.increment->accept(*this);}
  loop_depth++;
  node.body->accept(*this);
  loop_depth--;
  pop_scope();
}

void SemanticAnalyzer::visit(BreakStmt &node) {
  if (loop_depth == 0) {
    error(node.location, "'break' can only be used inside a loop");
  }
}

void SemanticAnalyzer::visit(ContinueStmt &node) {
  if (loop_depth == 0) {
    error(node.location, "'continue' can only be used inside a loop");
  }
}

void SemanticAnalyzer::error(SourceLocation loc, const std::string &message) {
  if (errors.size() < MAX_ERRORS) {
    errors.push_back(SemanticError{loc, message});
  }
}

void SemanticAnalyzer::push_scope() {
  scope_stack.push_back(current_scope);
  auto child = current_scope->create_child();
  current_scope = child.get();
  scope_storage.push_back(std::move(child));
}

void SemanticAnalyzer::pop_scope() {
  assert(!scope_stack.empty() && "Scope stack underflow");
  current_scope = scope_stack.back();
  scope_stack.pop_back();
}

Type *SemanticAnalyzer::get_expression_type(Expression &expr) {
  return expr.resolved_type.get();
}

bool SemanticAnalyzer::types_equal(const Type *a, const Type *b) {
  if (!a || !b) {
    return false;
  }

  return a->kind == b->kind;
}

bool SemanticAnalyzer::verify_all_paths_return(BlockStmt &block,
                                               Type *return_type) {
  if (return_type->kind == Type::Kind::VOID) {
    return true;
  }

  if (block.statements.empty()) {
    return false;
  }

  auto *last = block.statements.back().get();

  if (last->kind == Statement::Kind::RETURN) {return true;}

  if (last->kind == Statement::Kind::IF) {
    auto *if_stmt = static_cast<IfStmt*>(last);
    if (if_stmt->else_branch) {
      return verify_all_paths_return(*if_stmt->then_branch, return_type) &&
             verify_all_paths_return(*if_stmt->else_branch, return_type);
    }
  }

  return false;
}