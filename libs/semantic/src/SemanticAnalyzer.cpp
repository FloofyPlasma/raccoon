#include "raccoon/SemanticAnalyzer.hpp"
#include <print>

SemanticAnalyzer::SemanticAnalyzer()
    : global_scope(std::make_unique<SymbolTable>()),
      current_scope(global_scope.get()) {}

std::expected<void, std::vector<SemanticError>>
SemanticAnalyzer::analyze(ProgramNode &program) {
  errors.clear();
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

  if (node.body) {
    node.body->accept(*this);

    // Verify all paths return
    if (!verify_all_paths_return(*node.body, node.return_type.get())) {
      error(node.location, "Expected return statement in non-void function '" +
                               node.name + "'");
    }
  }

  current_function_return_type = nullptr;
}

void SemanticAnalyzer::visit(ReturnStmt &node) {
  // Check return value
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
      // Return with no value
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

void SemanticAnalyzer::error(SourceLocation loc, const std::string &message) {
  if (errors.size() < MAX_ERRORS) {
    errors.push_back(SemanticError{loc, message});
  }
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