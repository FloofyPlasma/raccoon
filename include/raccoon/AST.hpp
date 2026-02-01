#pragma once

#include <memory>
#include <vector>

class ASTNode {
public:
  virtual ~ASTNode() = default;
};

class ProgramNode : public ASTNode {
public:
};
