#include "raccoon/IRGenerator.hpp"
#include <cassert>
#include <print>

IRGenerator::IRGenerator()
    : last_value(IRValue::Kind::CONSTANT, IRType(IRType::Kind::VOID)) {}

std::unique_ptr<IRModule> IRGenerator::generate(ProgramNode &program,
                                                const std::string module_name) {
  module = std::make_unique<IRModule>(module_name);
  next_register = 0;
  next_label = 0;

  visit(program);

  return std::move(module);
}

void IRGenerator::visit(ProgramNode &node) {
  for (auto &func : node.functions) {
    func->accept(*this);
  }
}

void IRGenerator::visit(FunctionDecl &node) {
  IRType return_type = convert_type(node.return_type.get());
  auto func = std::make_unique<IRFunction>(node.name, return_type);

  // TODO: Params

  auto entry_block = std::make_unique<IRBasicBlock>("entry");
  func->basic_blocks.push_back(std::move(entry_block));

  current_function = func.get();
  current_block = func->basic_blocks.back().get();

  if (node.body) {
    node.body->accept(*this);
  }

  module->functions.push_back(std::move(func));
  current_function = nullptr;
  current_block = nullptr;
}

void IRGenerator::visit(BlockStmt &node) {
  for (auto &stmt : node.statements) {
    stmt->accept(*this);
  }
}

void IRGenerator::visit(ReturnStmt &node) {
  auto ret_instr = std::make_unique<IRInstruction>(IRInstruction::OpCode::RET);

  if (node.value) {
    node.value->accept(*this);

    ret_instr->operands.push_back(last_value);
  }

  emit(std::move(ret_instr));
}

void IRGenerator::visit(IntegerLiteral &node) {
  last_value = IRValue::make_constant(IRType(IRType::Kind::I32), node.value);
}

void IRGenerator::visit(BinaryExpr &node) {
  node.left->accept(*this);
  IRValue left_val = last_value;

  node.right->accept(*this);
  IRValue right_val = last_value;

  IRInstruction::OpCode opcode;
  switch (node.op) {
  case TokenType::PLUS:
    opcode = IRInstruction::OpCode::ADD;
    break;
  case TokenType::MINUS:
    opcode = IRInstruction::OpCode::SUB;
    break;
  case TokenType::STAR:
    opcode = IRInstruction::OpCode::MUL;
    break;
  case TokenType::SLASH:
    opcode = IRInstruction::OpCode::SDIV;
    break;
  default:
    assert(false && "Unknown binary operator in IR generation");
    return;
  }

  auto instr = std::make_unique<IRInstruction>(opcode);
  IRValue result = new_register(left_val.type);
  instr->result = result;
  instr->operands.push_back(left_val);
  instr->operands.push_back(right_val);

  emit(std::move(instr));
  last_value = result;
}

void IRGenerator::visit(IdentifierExpr &node) {
  auto it = variables.find(node.name);
  assert(it != variables.end() &&
         "Undefined variable should have been caught in semantic analysis");

  IRValue ptr = it->second;

  auto load_instr =
      std::make_unique<IRInstruction>(IRInstruction::OpCode::LOAD);

  IRType value_type = ptr.type;
  IRValue result = new_register(value_type);
  load_instr->result = result;
  load_instr->operands.push_back(ptr);

  emit(std::move(load_instr));
  last_value = result;
}

void IRGenerator::visit(VarDeclStmt &node) {
  IRType var_type = convert_type(node.type.get());

  auto alloca_instr = std::make_unique<IRInstruction>(IRInstruction::OpCode::ALLOCA);
  IRValue ptr = new_register(var_type);
  alloca_instr->result = ptr;

  emit(std::move(alloca_instr));

  variables.emplace(node.name, ptr);

  if (node.initializer) {
    node.initializer->accept(*this);

    auto store_instr = std::make_unique<IRInstruction>(IRInstruction::OpCode::STORE);
    store_instr->operands.push_back(last_value);
    store_instr->operands.push_back(ptr);

    emit(std::move(store_instr));
  }
}

void IRGenerator::visit(AssignmentStmt &node) {
  auto it = variables.find(node.name);
  assert(it != variables.end() && "Undefined variable should have been caught in semantic analysis");

  node.value->accept(*this);

  auto store_instr = std::make_unique<IRInstruction>(IRInstruction::OpCode::STORE);
  store_instr->operands.push_back(last_value);
  store_instr->operands.push_back(it->second);

  emit(std::move(store_instr));
}

void IRGenerator::visit(ExpressionStmt &node) {
  node.expression->accept(*this);
}

IRValue IRGenerator::new_register(IRType type) {
  std::string name = std::to_string(next_register++);
  return IRValue::make_register(type, name);
}

std::string IRGenerator::new_label(const std::string &prefix) {
  return prefix + std::to_string(next_label++);
}

IRType IRGenerator::convert_type(const Type *ast_type) {
  if (!ast_type) {
    return IRType(IRType::Kind::VOID);
  }

  switch (ast_type->kind) {
  case Type::Kind::I32:
    return IRType(IRType::Kind::I32);
  case Type::Kind::VOID:
    return IRType(IRType::Kind::VOID);
  }

  // Should never reach here if semantic analysis passed
  return IRType(IRType::Kind::VOID);
}

void IRGenerator::emit(std::unique_ptr<IRInstruction> instr) {
  if (current_block) {
    current_block->instructions.push_back(std::move(instr));
  }
}