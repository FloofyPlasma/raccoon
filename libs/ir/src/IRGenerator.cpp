#include "raccoon/IRGenerator.hpp"
#include <cassert>
#include <print>

IRGenerator::IRGenerator()
    : last_value(IRValue::Kind::CONSTANT, IRType(IRType::Kind::VOID)) {}

std::unique_ptr<IRModule> IRGenerator::generate(ProgramNode &program,
                                                const std::string module_name) {
  module = std::make_unique<IRModule>(module_name);

  visit(program);

  return std::move(module);
}

void IRGenerator::visit(ProgramNode &node) {
  for (auto &func : node.functions) {
    func->accept(*this);
  }
}

void IRGenerator::visit(FunctionDecl &node) {
  auto ir_func = std::make_unique<IRFunction>(
      node.name, convert_type(node.return_type.get()));

  current_function = ir_func.get();
  next_register = 0;
  next_label = 0;
  variables.clear();
  loop_stack.clear();

  auto entry = std::make_unique<IRBasicBlock>("entry");
  current_block = entry.get();
  current_function->basic_blocks.push_back(std::move(entry));

  if (node.body) {
    for (auto &stmt : node.body->statements) {
      stmt->accept(*this);
    }
  }

  module->functions.push_back(std::move(ir_func));
  current_function = nullptr;
  current_block = nullptr;
}

void IRGenerator::visit(BlockStmt &node) {
  for (auto &stmt : node.statements) {
    if (current_block_has_terminator()) {
      break;
    }
    stmt->accept(*this);
  }
}

void IRGenerator::visit(ReturnStmt &node) {
  if (current_block_has_terminator()) {
    return;
  }

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

void IRGenerator::visit(BoolLiteral &node) {
  last_value =
      IRValue::make_constant(IRType(IRType::Kind::BOOL), node.value ? 1 : 0);
}

void IRGenerator::visit(BinaryExpr &node) {
  node.left->accept(*this);
  IRValue left_val = last_value;

  node.right->accept(*this);
  IRValue right_val = last_value;

  IRInstruction::OpCode opcode;
  IRType result_type(IRType::Kind::I32);

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
  case TokenType::LESS:
    opcode = IRInstruction::OpCode::ICMP_SLT;
    result_type = IRType(IRType::Kind::BOOL);
    break;
  case TokenType::GREATER:
    opcode = IRInstruction::OpCode::ICMP_SGT;
    result_type = IRType(IRType::Kind::BOOL);
    break;
  case TokenType::LESS_EQUAL:
    opcode = IRInstruction::OpCode::ICMP_SLE;
    result_type = IRType(IRType::Kind::BOOL);
    break;
  case TokenType::GREATER_EQUAL:
    opcode = IRInstruction::OpCode::ICMP_SGE;
    result_type = IRType(IRType::Kind::BOOL);
    break;
  case TokenType::EQUAL_EQUAL:
    opcode = IRInstruction::OpCode::ICMP_EQ;
    result_type = IRType(IRType::Kind::BOOL);
    break;
  case TokenType::BANG_EQUAL:
    opcode = IRInstruction::OpCode::ICMP_NE;
    result_type = IRType(IRType::Kind::BOOL);
    break;
  case TokenType::AND_AND:
    opcode = IRInstruction::OpCode::AND;
    result_type = IRType(IRType::Kind::BOOL);
    break;
  case TokenType::OR_OR:
    opcode = IRInstruction::OpCode::OR;
    result_type = IRType(IRType::Kind::BOOL);
    break;
  default:
    assert(false && "Unknown binary operator in IR generation");
    return;
  }

  auto instr = std::make_unique<IRInstruction>(opcode);
  IRValue result = new_register(result_type);
  instr->result = result;
  instr->operands.push_back(left_val);
  instr->operands.push_back(right_val);

  emit(std::move(instr));
  last_value = result;
}

void IRGenerator::visit(UnaryExpr &node) {
  node.operand->accept(*this);
  IRValue operand = last_value;

  switch (node.op) {
  case TokenType::MINUS: {
    auto instr = std::make_unique<IRInstruction>(IRInstruction::OpCode::NEG);
    instr->result = new_register(IRType(IRType::Kind::I32));
    instr->operands.push_back(operand);
    last_value = instr->result;
    emit(std::move(instr));
    break;
  }

  case TokenType::BANG: {
    auto instr = std::make_unique<IRInstruction>(IRInstruction::OpCode::NOT);
    instr->result = new_register(IRType(IRType::Kind::BOOL));
    instr->operands.push_back(operand);
    last_value = instr->result;
    emit(std::move(instr));
    break;
  }

  default:
    assert(false && "Unknown unary operator in IR generation");
  }
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

  auto alloca_instr =
      std::make_unique<IRInstruction>(IRInstruction::OpCode::ALLOCA);
  IRValue ptr = new_register(var_type);
  alloca_instr->result = ptr;

  emit(std::move(alloca_instr));

  variables.emplace(node.name, ptr);

  if (node.initializer) {
    node.initializer->accept(*this);

    auto store_instr =
        std::make_unique<IRInstruction>(IRInstruction::OpCode::STORE);
    store_instr->operands.push_back(last_value);
    store_instr->operands.push_back(ptr);

    emit(std::move(store_instr));
  }
}

void IRGenerator::visit(AssignmentStmt &node) {
  auto it = variables.find(node.name);
  assert(it != variables.end() &&
         "Undefined variable should have been caught in semantic analysis");

  node.value->accept(*this);

  auto store_instr =
      std::make_unique<IRInstruction>(IRInstruction::OpCode::STORE);
  store_instr->operands.push_back(last_value);
  store_instr->operands.push_back(it->second);

  emit(std::move(store_instr));
}

void IRGenerator::visit(ExpressionStmt &node) {
  node.expression->accept(*this);
}

void IRGenerator::visit(IfStmt &node) {
  node.condition->accept(*this);
  IRValue cond = last_value;

  std::string then_label = new_label("then");
  std::string else_label = node.else_branch ? new_label("else") : "";
  std::string merge_label = new_label("if.merge");

  auto br = std::make_unique<IRInstruction>(IRInstruction::OpCode::BR_COND);
  br->operands.push_back(cond);
  br->operands.push_back(IRValue::make_label(then_label));
  br->operands.push_back(
      IRValue::make_label(node.else_branch ? else_label : merge_label));
  emit(std::move(br));

  switch_to_block(then_label);
  for (auto &stmt : node.then_branch->statements) {
    if (current_block_has_terminator()) {
      break;
    }
    stmt->accept(*this);
  }
  if (!current_block_has_terminator()) {
    auto br_merge = std::make_unique<IRInstruction>(IRInstruction::OpCode::BR);
    br_merge->operands.push_back(IRValue::make_label(merge_label));
    emit(std::move(br_merge));
  }

  if (node.else_branch) {
    switch_to_block(else_label);
    for (auto &stmt : node.else_branch->statements) {
      if (current_block_has_terminator()) {
        break;
      }
      stmt->accept(*this);
    }
    if (!current_block_has_terminator()) {
      auto br_merge =
          std::make_unique<IRInstruction>(IRInstruction::OpCode::BR);
      br_merge->operands.push_back(IRValue::make_label(merge_label));
      emit(std::move(br_merge));
    }
  }

  switch_to_block(merge_label);
}

void IRGenerator::visit(WhileStmt &node) {
  std::string cond_label = new_label("while.cond");
  std::string body_label = new_label("while.body");
  std::string exit_label = new_label("while.exit");

  if (!current_block_has_terminator()) {
    auto br = std::make_unique<IRInstruction>(IRInstruction::OpCode::BR);
    br->operands.push_back(IRValue::make_label(cond_label));
    emit(std::move(br));
  }

  switch_to_block(cond_label);
  node.condition->accept(*this);
  IRValue cond = last_value;

  auto br_cond =
      std::make_unique<IRInstruction>(IRInstruction::OpCode::BR_COND);
  br_cond->operands.push_back(cond);
  br_cond->operands.push_back(IRValue::make_label(body_label));
  br_cond->operands.push_back(IRValue::make_label(exit_label));
  emit(std::move(br_cond));

  switch_to_block(body_label);
  loop_stack.push_back({cond_label, exit_label});
  for (auto &stmt : node.body->statements) {
    if (current_block_has_terminator()) {
      break;
    }
    stmt->accept(*this);
  }
  loop_stack.pop_back();

  if (!current_block_has_terminator()) {
    auto br_back = std::make_unique<IRInstruction>(IRInstruction::OpCode::BR);
    br_back->operands.push_back(IRValue::make_label(exit_label));
    emit(std::move(br_back));
  }

  switch_to_block(exit_label);
}

void IRGenerator::visit(ForStmt &node) {
  if (node.initializer) {
    node.initializer->accept(*this);
  }

  std::string cond_label = new_label("for.cond");
  std::string body_label = new_label("for.body");
  std::string incr_label = new_label("for.incr");
  std::string exit_label = new_label("for.exit");

  if (!current_block_has_terminator()) {
    auto br = std::make_unique<IRInstruction>(IRInstruction::OpCode::BR);
    br->operands.push_back(IRValue::make_label(cond_label));
    emit(std::move(br));
  }

  switch_to_block(cond_label);
  if (node.condition) {
    node.condition->accept(*this);
    IRValue cond = last_value;
    auto br_cond =
        std::make_unique<IRInstruction>(IRInstruction::OpCode::BR_COND);
    br_cond->operands.push_back(cond);
    br_cond->operands.push_back(IRValue::make_label(body_label));
    br_cond->operands.push_back(IRValue::make_label(exit_label));
    emit(std::move(br_cond));
  } else {
    auto br = std::make_unique<IRInstruction>(IRInstruction::OpCode::BR);
    br->operands.push_back(IRValue::make_label(body_label));
    emit(std::move(br));
  }

  switch_to_block(body_label);
  loop_stack.push_back({incr_label, exit_label});
  for (auto &stmt : node.body->statements) {
    if (current_block_has_terminator()) {
      break;
    }
    stmt->accept(*this);
  }
  loop_stack.pop_back();

  if (!current_block_has_terminator()) {
    auto br_incr = std::make_unique<IRInstruction>(IRInstruction::OpCode::BR);
    br_incr->operands.push_back(IRValue::make_label(incr_label));
    emit(std::move(br_incr));
  }

  switch_to_block(incr_label);
  if (node.increment) {
    node.increment->accept(*this);
  }
  if (!current_block_has_terminator()) {
    auto br_cond = std::make_unique<IRInstruction>(IRInstruction::OpCode::BR);
    br_cond->operands.push_back(IRValue::make_label(cond_label));
    emit(std::move(br_cond));
  }

  switch_to_block(exit_label);
}

void IRGenerator::visit(BreakStmt &node) {
  (void)node;
  assert(!loop_stack.empty() &&
         "Break outside loop should have been caught by semantic");
  if (current_block_has_terminator()) {
    return;
  }
  auto br = std::make_unique<IRInstruction>(IRInstruction::OpCode::BR);
  br->operands.push_back(IRValue::make_label(loop_stack.back().break_label));
  emit(std::move(br));
}

void IRGenerator::visit(ContinueStmt &node) {
  (void)node;
  assert(!loop_stack.empty() &&
         "Continue outside loop should have been caught by semantic");
  if (current_block_has_terminator()) {
    return;
  }
  auto br = std::make_unique<IRInstruction>(IRInstruction::OpCode::BR);
  br->operands.push_back(IRValue::make_label(loop_stack.back().continue_label));
  emit(std::move(br));
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
  case Type::Kind::BOOL:
    return IRType(IRType::Kind::BOOL);
  case Type::Kind::VOID:
    return IRType(IRType::Kind::VOID);
  }

  return IRType(IRType::Kind::VOID);
}

void IRGenerator::emit(std::unique_ptr<IRInstruction> instr) {
  assert(current_block && "No current block to emit into");
  current_block->instructions.push_back(std::move(instr));
}

bool IRGenerator::current_block_has_terminator() const {
  if (!current_block || current_block->instructions.empty()) {
    return false;
  }
  auto &last = current_block->instructions.back();
  return last->opcode == IRInstruction::OpCode::RET ||
         last->opcode == IRInstruction::OpCode::BR ||
         last->opcode == IRInstruction::OpCode::BR_COND;
}

void IRGenerator::switch_to_block(const std::string &label) {
  auto block = std::make_unique<IRBasicBlock>(label);
  current_block = block.get();
  current_function->basic_blocks.push_back(std::move(block));
}