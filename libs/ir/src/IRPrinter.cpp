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

  case IRInstruction::OpCode::ALLOCA:
    std::print("{} = alloca {}", instr.result.to_string(),
               instr.result.type.to_string());
    break;

  case IRInstruction::OpCode::LOAD:
    std::print("{} = load {}, {}* {}", instr.result.to_string(),
               instr.result.type.to_string(),
               instr.operands[0].type.to_string(),
               instr.operands[0].to_string());
    break;

  case IRInstruction::OpCode::STORE:
    std::print("store {} {}, {}* {}", instr.operands[0].type.to_string(),
               instr.operands[0].to_string(),
               instr.operands[1].type.to_string(),
               instr.operands[1].to_string());
    break;

  case IRInstruction::OpCode::ADD:
  case IRInstruction::OpCode::SUB:
  case IRInstruction::OpCode::MUL:
  case IRInstruction::OpCode::SDIV:
  case IRInstruction::OpCode::AND:
  case IRInstruction::OpCode::OR:
  case IRInstruction::OpCode::ICMP_EQ:
  case IRInstruction::OpCode::ICMP_NE:
  case IRInstruction::OpCode::ICMP_SLT:
  case IRInstruction::OpCode::ICMP_SGT:
  case IRInstruction::OpCode::ICMP_SLE:
  case IRInstruction::OpCode::ICMP_SGE:
    std::print("{} = {} {} {}, {}", instr.result.to_string(),
               instr.opcode_to_string(), instr.result.type.to_string(),
               instr.operands[0].to_string(), instr.operands[1].to_string());
    break;

  case IRInstruction::OpCode::NEG:
  case IRInstruction::OpCode::NOT:
    std::print("{} = {} {} {}", instr.result.to_string(),
               instr.opcode_to_string(), instr.result.type.to_string(),
               instr.operands[0].to_string());

  case IRInstruction::OpCode::RET:
    std::print("ret");
    if (!instr.operands.empty()) {
      std::print(" {} {}", instr.operands[0].type.to_string(),
                 instr.operands[0].to_string());
    }
    break;

  case IRInstruction::OpCode::BR:
    std::print("br label %{}", instr.operands[0].to_string());
    break;

  case IRInstruction::OpCode::BR_COND:
    std::print("br {} {}, label %{}, label %{}",
               instr.operands[0].type.to_string(),
               instr.operands[0].to_string(), instr.operands[1].to_string(),
               instr.operands[2].to_string());
    break;
  }
}