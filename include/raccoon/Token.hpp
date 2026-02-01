#pragma once

#include <string>
#include <string_view>
#include <optional>
#include <cstdint>

enum class TokenType {
  // Keywords
  FUN,
  RETURN,

  // Types
  I32,

  // Literals
  IDENTIFIER,
  INTEGER,

  // Punctuation
  LEFT_PAREN,     // (
  RIGHT_PAREN,    // )
  LEFT_BRACE,     // {
  RIGHT_BRACE,    // }
  COLON,          // :
  SEMICOLON,      // ;

  // Special
  END_OF_FILE
};

struct SourceLocation {
  std::string filename;
  int line;
  int column;
};

struct Token {
  TokenType type;
  std::string_view lexeme; // Points into source string
  SourceLocation location;

  // For literals that need parsed values
  std::optional<std::int64_t> int_value;
};

std::string token_type_to_string(TokenType type);