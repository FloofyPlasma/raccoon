#include "raccoon/IRGenerator.hpp"
#include "raccoon/AST.hpp"
#include "raccoon/IR.hpp"

std::unique_ptr<IRModule> IRGenerator::generate(ProgramNode *program) {
  (void)program;

  auto module = std::make_unique<IRModule>();
  module->name = "main";
  return module;
}
