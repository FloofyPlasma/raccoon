#pragma once

#include <memory>
#include <vector>

struct Token;
class ProgramNode;

class Parser {
public:
  explicit Parser(std::vector<Token> tokens);

  std::unique_ptr<ProgramNode> parse();

private:
  std::vector<Token> tokens;
};
