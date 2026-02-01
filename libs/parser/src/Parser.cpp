#include "raccoon/Parser.hpp"
#include "raccoon/AST.hpp"
#include "raccoon/Lexer.hpp"

Parser::Parser(std::vector<Token> tokens) : tokens(std::move(tokens)) {}

std::unique_ptr<ProgramNode> Parser::parse() {
  return std::make_unique<ProgramNode>();
}
