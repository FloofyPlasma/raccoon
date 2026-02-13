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

  visit(program);

  if (!errors.empty()) {
    return std::unexpected(std::move(errors));
  }

  return {};
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
        error(node.location,
              "Cannot return value of type '" +
                  std::string(value_type->kind == Type::Kind::I32 ? "i32"
                                                                  : "void") +
                  "' from function with return type '" +
                  std::string(current_function_return_type->kind ==
                                      Type::Kind::I32
                                  ? "i32"
                                  : "void") +
                  "'");
      }
    } else {
      if (current_function_return_type &&
          current_function_return_type->kind != Type::Kind::VOID) {
        error(node.location,
              "Cannot return without value from non-void function");
      }
    }
  }
}

void SemanticAnalyzer::visit(IntegerLiteral &node) {
  node.resolved_type = std::make_unique<Type>(Type::Kind::I32, node.location);
}

void SemanticAnalyzer::visit(BinaryExpr &node) {
  node.left->accept(*this);
  node.right->accept(*this);

  Type *left_type = get_expression_type(*node.left);
  Type *right_type = get_expression_type(*node.right);

  if (left_type && right_type) {
    if (!types_equal(left_type, right_type)) {
      error(node.location, "Type mismatch in binary expression");
    }

    node.resolved_type = std::make_unique<Type>(left_type->kind, node.location);
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
    error(node.location, "Variable '" + node.name +"' is already defined in this scope");
    return;
  }

  if (node.initializer) {
    node.initializer->accept(*this);

    Type *init_type = get_expression_type(*node.initializer);
    if (init_type && node.type) {
      if (!types_equal(init_type, node.type.get())) {
        error(node.location, "Type mismatch in variable initialization");
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

  Type* value_type = get_expression_type(*node.value);
  if (value_type && sym->type) {
    if (!types_equal(value_type, sym->type)) {
      error(node.location, "Type mismatch in assignment to '" + node.name + "'");
    }
  }
}

void SemanticAnalyzer::visit(ExpressionStmt &node) {
  node.expression->accept(*this);
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

  auto *last_stmt = block.statements.back().get();
  return last_stmt->kind == Statement::Kind::RETURN;
}