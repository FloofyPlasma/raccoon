#include "raccoon/IRPrinter.hpp"

void IRPrinter::print(const IRModule &module) {
  std::println("module @{}\n", module.name);

  for (const auto &func : module.functions) {
    print_function(*func);
    std::println("");
  }
}

void IRPrinter::print_function(const IRFunction &func) {
  std::print("define {} @{}(", func.return_type.to_string(), func.name);

  // TODO: Params

  std::println(") {{");

  for (const auto &block : func.basic_blocks) {
    print_basic_block(*block);
  }

  std::println("}}");
}

void IRPrinter::print_basic_block(const IRBasicBlock &block) {
  std::println("{}:", block.label);

  for (const auto &instr : block.instructions) {
    std::print("    ");
    print_instruction(*instr);
    std::println("");
  }
}

void IRPrinter::print_instruction(const IRInstruction &instr) {
  switch (instr.opcode) {
  case IRInstruction::OpCode::RET:
    std::print("ret");
    if (!instr.operands.empty()) {
      std::print(" {} {}", instr.operands[0].type.to_string(),
                 instr.operands[0].to_string());
    }
    break;

  case IRInstruction::OpCode::ADD:
  case IRInstruction::OpCode::SUB:
  case IRInstruction::OpCode::MUL:
  case IRInstruction::OpCode::SDIV:
  case IRInstruction::OpCode::ALLOCA:
  case IRInstruction::OpCode::LOAD:
  case IRInstruction::OpCode::STORE:
  case IRInstruction::OpCode::BR:
  case IRInstruction::OpCode::BR_COND:
    std::print("{}  ; Unimplemented opcode", instr.opcode_to_string());
    break;
  }
}