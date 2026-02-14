#include "raccoon/IR.hpp"
#include <print>

#pragma mark IR Type

std::string IRType::to_string() const {
  switch (kind) {
  case Kind::I32:
    return "i32";
  case Kind::BOOL:
    return "bool";
  case Kind::VOID:
    return "void";
  }
  return "unknown";
}

#pragma mark IR Value

IRValue IRValue::make_register(IRType type, const std::string &name) {
  IRValue val(Kind::REGISTER, type);
  val.name = name;
  return val;
}

IRValue IRValue::make_constant(IRType type, int64_t value) {
  IRValue val(Kind::CONSTANT, type);
  val.int_const = value;
  return val;
}

IRValue IRValue::make_label(const std::string &name) {
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
  case OpCode::NEG: return "neg";
  case OpCode::ICMP_EQ: return "icmp_eq";
  case OpCode::ICMP_NE: return "icmp_ne";
  case OpCode::ICMP_SLT: return "icmp_slt";
  case OpCode::ICMP_SGT: return "icmp_sgt";
  case OpCode::ICMP_SLE: return "icmp_sle";
  case OpCode::ICMP_SGE: return "icmp_sge";
  case OpCode::AND: return "and";
  case OpCode::OR: return "or";
  case OpCode::NOT: return "not";
  case OpCode::RET: return "ret";
  case OpCode::BR: return "br";
  case OpCode::BR_COND: return "br_cond";
  }
  return "unknown";
}