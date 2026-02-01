#pragma once

#include "raccoon/Token.hpp"

#include <string>
#include <vector>

struct LexerError {
  SourceLocation location;
  std::string message;
};

class Lexer {
public:
  Lexer(std::string source, std::string filename);

  // Returns tokens if successful, errors otherwise
  std::vector<Token> lex();

  bool has_errors() const { return !errors.empty(); }

  const std::vector<LexerError> &get_errors() const { return errors; }

private:
  std::string source;
  std::string filename;
  std::size_t position = 0;
  int line = 1;
  int column = 1;

  std::vector<LexerError> errors;

  bool is_at_end() const;
  char peek() const;
  char peek_next() const;
  char advance();
  void skip_whitespace();

  Token scan_token();
  Token scan_identifier_or_keyword();
  Token scan_number();

  SourceLocation current_location() const;

  void error(const std::string &message);

  bool is_alpha(char c) const;
  bool is_digit(char c) const;
  bool is_alnum(char c) const;
};
