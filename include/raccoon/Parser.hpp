#pragma once

#include "raccoon/AST.hpp"
#include "raccoon/Token.hpp"
#include <expected>
#include <memory>
#include <string>
#include <vector>

struct ParserError {
  SourceLocation location;
  std::string message;
};

class Parser {
public:
  explicit Parser(std::vector<Token> tokens);

  std::expected<std::unique_ptr<ProgramNode>, std::vector<ParserError>> parse();

private:
  std::vector<Token> tokens;
  std::size_t current = 0;
  std::vector<ParserError> errors;

  static constexpr std::size_t MAX_ERRORS = 20;

  // Utils

  bool is_at_end() const;
  const Token& peek() const;
  const Token& previous() const;
  const Token& advance();
  bool check(TokenType type) const;
  bool match(TokenType type);

  void error(const std::string& message);
  void synchronize();

  bool expect(TokenType type, const std::string& message);

  // Parsing functions
  std::unique_ptr<FunctionDecl> parse_function();
  std::unique_ptr<Type> parse_type();
  std::unique_ptr<BlockStmt> parse_block();
  std::unique_ptr<Statement> parse_statement();
  std::unique_ptr<Expression> parse_expression();
  std::unique_ptr<Expression> parse_primary();
};
