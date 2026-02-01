#pragma once

#include <memory>

class ProgramNode;
class IRModule;

class IRGenerator {
public:
  IRGenerator() = default;

  std::unique_ptr<IRModule> generate(ProgramNode *program);
};
