#include "raccoon/SymbolTable.hpp"

SymbolTable::SymbolTable(SymbolTable *parent) :parent(parent) {  }

bool SymbolTable::insert(const std::string &name,
                         std::unique_ptr<Symbol> symbol) {
  if (symbols.contains(name)) {
    return false;
  }

  symbols[name] = std::move(symbol);
  return true;
}

Symbol *SymbolTable::lookup(const std::string &name) {
  if (auto it = symbols.find(name); it != symbols.end()) {
    return it->second.get();
  }

  if (parent) {
    return parent->lookup(name);
  }

  return nullptr;
}

Symbol *SymbolTable::lookup_local(const std::string &name) {
  if (auto it = symbols.find(name); it != symbols.end()) {
    return it->second.get();
  }

  return nullptr;
}

std::unique_ptr<SymbolTable> SymbolTable::create_child() {
  return std::make_unique<SymbolTable>(this);
}

std::string SymbolTable::get_scope_path() const {
  if (parent) {
    std::string parent_path = parent->get_scope_path();
    if (!parent_path.empty()) {
      return parent_path + "::child";
    }
    return "child";
  }
  return "global";
}