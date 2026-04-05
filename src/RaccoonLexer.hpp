#pragma once

#include <arc/lexer.hpp>
#include <string>

#include "RaccoonToken.hpp"

inline arc::LexerConfig raccoon_lexer_config()
{
  return arc::LexerConfig {
    .line_comments = { "//" },
    .block_comments = { { "/*", "*/" } },
    .hex_literals = true,
    .binary_literals = true,
    .octal_literals = true,
    .float_suffixes = true,
    .scientific_notation = true,
    .escape_sequences = true,
    .max_errors = 10,
  };
}

class RaccoonLexer
{
public:
  explicit RaccoonLexer(std::string source, std::string filename) :
      lexer_(std::move(source), std::move(filename), raccoon_lexer_config())
  {
  }

  arc::LexResult<RaccoonToken> lex() { return lexer_.lex(); }

  arc::Token<RaccoonToken> next() { return lexer_.next(); }

  const arc::Token<RaccoonToken> &peek() { return lexer_.peek(); }

  bool is_at_end() const { return lexer_.is_at_end(); }
  arc::SourceLocation location() const { return lexer_.location(); }
  bool has_errors() const { return lexer_.has_errors(); }

  const std::vector<arc::Diagnostic> &diagnostics() const { return lexer_.diagnostics(); }

private:
  arc::Lexer<RaccoonToken> lexer_;
};
