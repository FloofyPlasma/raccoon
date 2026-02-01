#pragma once

class ProgramNode;

class SemanticAnalyzer {
public:
  SemanticAnalyzer() = default;

  bool analyze(ProgramNode *program);
};
