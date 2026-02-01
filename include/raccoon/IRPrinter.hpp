#pragma once

#include "raccoon/IR.hpp"
#include <print>

class IRPrinter {
public:
  void print(const IRModule &module);

private:
  void print_function(const IRFunction &func);
  void print_basic_block(const IRBasicBlock &block);
  void print_instruction(const IRInstruction &instr);
};