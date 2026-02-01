#pragma once

#include "raccoon/AST.hpp"
#include <unordered_map>
#include <memory>
#include <string>

enum class SymbolKind {
  FUNCTION,
  VARIABLE,
  PARAMETER
};

struct Symbol {
  std::string name;
  SymbolKind kind;
  Type* type; // Non-owning pointer to type in AST
  SourceLocation location;

  void* ir_value = nullptr;
};

class SymbolTable {
public:
  explicit SymbolTable(SymbolTable* parent = nullptr);

  // Insert symbol (returns false if already exists in current scope)
  bool insert(const std::string& name, std::unique_ptr<Symbol> symbol);

  // Lookup (searches in parent scope)
  Symbol* lookup(const std::string& name);

  // Lookup only in current scope
  Symbol* lookup_local(const std::string& name);

  // Create child scope
  std::unique_ptr<SymbolTable> create_child();

  // Get scope path for error messages
  std::string get_scope_path() const;

private:
  SymbolTable* parent;
  std::unordered_map<std::string, std::unique_ptr<Symbol>> symbols;
};