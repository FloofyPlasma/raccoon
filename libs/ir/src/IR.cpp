#include "raccoon/IR.hpp"
#include <print>

#pragma mark IR Type

std::string IRType::to_string() const {
  switch (kind) {
  case Kind::I32: return "i32";
  case Kind::VOID: return "void";
  }
  return "unknown";
}

#pragma mark IR Value

IRValue IRValue::make_register(IRType type, const std::string& name) {
  IRValue val(Kind::REGISTER, type);
  val.name = name;
  return val;
}

IRValue IRValue::make_constant(IRType type, int64_t value) {
  IRValue val(Kind::CONSTANT, type);
  val.int_const = value;
  return val;
}

IRValue IRValue::make_label(const std::string& name) {
  IRValue val(Kind::LABEL, IRType(IRType::Kind::VOID));
  val.name = name;
  return val;
}

std::string IRValue::to_string() const {
  switch (kind) {
  case Kind::REGISTER:
    return "%" + name;
  case Kind::CONSTANT:
    return std::format("{}", int_const);
  case Kind::LABEL:
    return name;
  }
  return "unknown";
}

#pragma mark IR Instruction


std::string IRInstruction::opcode_to_string() const {
  switch (opcode) {
  case OpCode::ALLOCA: return "alloca";
  case OpCode::LOAD: return "load";
  case OpCode::STORE: return "store";
  case OpCode::ADD: return "add";
  case OpCode::SUB: return "sub";
  case OpCode::MUL: return "mul";
  case OpCode::SDIV: return "sdiv";
  case OpCode::RET: return "ret";
  case OpCode::BR: return "br";
  case OpCode::BR_COND: return "br";
  }
  return "unknown";
}