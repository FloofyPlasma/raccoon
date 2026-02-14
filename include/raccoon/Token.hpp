#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

enum class TokenType {
  // Keywords
  FUN,
  RETURN,
  LET,
  IF,
  ELSE,
  WHILE,
  FOR,
  BREAK,
  CONTINUE,
  TRUE_KW,
  FALSE_KW,

  // Types
  I32,
  BOOL,

  // Literals
  IDENTIFIER,
  INTEGER,

  // Arithmetic Operators
  PLUS,  // +
  MINUS, // -
  STAR,  // *
  SLASH, // /
  EQUAL, // =

  // Comparison Operators
  LESS,          // <
  GREATER,       // >
  LESS_EQUAL,    // <=
  GREATER_EQUAL, // >=
  EQUAL_EQUAL,   // ==
  BANG_EQUAL,    // !=

  // Logical Operators
  AND_AND, // &&
  OR_OR,   // ||
  BANG,    // !

  // Punctuation
  LEFT_PAREN,  // (
  RIGHT_PAREN, // )
  LEFT_BRACE,  // {
  RIGHT_BRACE, // }
  COLON,       // :
  SEMICOLON,   // ;

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
  std::optional<std::int64_t> int_value = std::nullopt;
};

std::string token_type_to_string(TokenType type);