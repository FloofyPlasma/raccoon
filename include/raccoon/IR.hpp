#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#pragma mark IR Types

struct IRType {
  enum class Kind { I32, VOID };

  Kind kind;

  explicit IRType(Kind k) : kind(k) {}

  bool operator==(const IRType &other) const { return kind == other.kind; }

  bool operator!=(const IRType &other) const { return !(*this == other); }

  std::string to_string() const;
};

#pragma mark IR Values

struct IRValue {
  enum class Kind { REGISTER, CONSTANT, LABEL };

  Kind kind;
  IRType type;

  std::string name;
  std::int64_t int_const;

  IRValue(Kind k, IRType t) : kind(k), type(t), int_const(0) {}

  static IRValue make_register(IRType type, const std::string &name);
  static IRValue make_constant(IRType type, int64_t value);
  static IRValue make_label(const std::string &name);

  std::string to_string() const;
};

#pragma mark IR Instructions

struct IRInstruction {
  enum class OpCode {
    // Memory
    ALLOCA,
    LOAD,
    STORE,

    // Arithmetic
    ADD,
    SUB,
    MUL,
    SDIV,

    // Control Flow
    RET,
    BR,
    BR_COND
  };

  OpCode opcode;
  IRValue result;
  std::vector<IRValue> operands;

  explicit IRInstruction(OpCode op)
      : opcode(op),
        result(IRValue::Kind::REGISTER, IRType(IRType::Kind::VOID)) {}

  std::string opcode_to_string() const;
};

#pragma mark IR Basic Block

struct IRBasicBlock {
  std::string label;
  std::vector<std::unique_ptr<IRInstruction>> instructions;

  explicit IRBasicBlock(std::string lbl) : label(std::move(lbl)) {}
};

#pragma mark IR Function

struct IRParameter {
  std::string name;
  IRType type;
};

struct IRFunction {
  std::string name;
  IRType return_type;
  std::vector<IRParameter> parameters;
  std::vector<std::unique_ptr<IRBasicBlock>> basic_blocks;

  IRFunction(std::string n, IRType ret)
      : name(std::move(n)), return_type(ret) {}
};

#pragma mark IR Module

struct IRModule {
  std::string name;
  std::vector<std::unique_ptr<IRFunction>> functions;

  explicit IRModule(std::string n) : name(std::move(n)) {}
};
