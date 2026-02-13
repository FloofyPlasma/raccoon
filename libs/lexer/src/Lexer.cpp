#include "raccoon/Lexer.hpp"

#include <cctype>
#include <unordered_map>

std::string token_type_to_string(TokenType type) {
  switch (type) {
  case TokenType::FUN:
    return "FUN";
  case TokenType::RETURN:
    return "RETURN";
  case TokenType::LET:
    return "LET";
  case TokenType::I32:
    return "I32";
  case TokenType::IDENTIFIER:
    return "IDENTIFIER";
  case TokenType::INTEGER:
    return "INTEGER";
  case TokenType::PLUS:
    return "PLUS";
  case TokenType::MINUS:
    return "MINUS";
  case TokenType::STAR:
    return "STAR";
  case TokenType::SLASH:
    return "SLASH";
  case TokenType::EQUAL:
    return "EQUAL";
  case TokenType::LEFT_PAREN:
    return "LEFT_PAREN";
  case TokenType::RIGHT_PAREN:
    return "RIGHT_PAREN";
  case TokenType::LEFT_BRACE:
    return "LEFT_BRACE";
  case TokenType::RIGHT_BRACE:
    return "RIGHT_BRACE";
  case TokenType::COLON:
    return "COLON";
  case TokenType::SEMICOLON:
    return "SEMICOLON";
  case TokenType::END_OF_FILE:
    return "END_OF_FILE";
  default:
    return "UNKNOWN";
  }
}

Lexer::Lexer(std::string source, std::string filename)
    : source(std::move(source)), filename(std::move(filename)) {}

std::vector<Token> Lexer::lex() {
  std::vector<Token> tokens;
  errors.clear();

  while (!is_at_end()) {
    skip_whitespace();
    if (is_at_end()) {
      break;
    }

    Token token = scan_token();
    tokens.push_back(token);

    // Arbitrary error limit before emitting "too many errors"
    if (errors.size() >= 20) {
      break;
    }
  }

  tokens.push_back(Token{TokenType::END_OF_FILE, std::string_view{},
                         current_location(), std::nullopt});

  return tokens;
}

bool Lexer::is_at_end() const { return position >= source.length(); }

char Lexer::peek() const {
  if (is_at_end()) {
    return '\0';
  }

  return source[position];
}

char Lexer::peek_next() const {
  if (position + 1 >= source.length()) {
    return '\0';
  }

  return source[position + 1];
}

char Lexer::advance() {
  char c = source[position++];
  if (c == '\n') {
    line++;
    column = 1;
  } else {
    column++;
  }

  return c;
}

void Lexer::skip_whitespace() {
  while (!is_at_end()) {
    char c = peek();

    if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
      advance();
    } else if (c == '/' && peek_next() == '/') {
      // Skip line comment
      while (!is_at_end() && peek() != '\n') {
        advance();
      }
    } else {
      break;
    }
  }
}

Token Lexer::scan_token() {
  SourceLocation loc = current_location();
  std::size_t start = position;
  char c = advance();

  switch (c) {
  case '(':
    return Token{TokenType::LEFT_PAREN, std::string_view(&source[start], 1),
                 loc, std::nullopt};
  case ')':
    return Token{TokenType::RIGHT_PAREN, std::string_view(&source[start], 1),
                 loc, std::nullopt};
  case '{':
    return Token{TokenType::LEFT_BRACE, std::string_view(&source[start], 1),
                 loc, std::nullopt};
  case '}':
    return Token{TokenType::RIGHT_BRACE, std::string_view(&source[start], 1),
                 loc, std::nullopt};
  case ':':
    return Token{TokenType::COLON, std::string_view(&source[start], 1), loc,
                 std::nullopt};
  case ';':
    return Token{TokenType::SEMICOLON, std::string_view(&source[start], 1), loc,
                 std::nullopt};
  case '+':
    return Token{TokenType::PLUS, std::string_view(&source[start], 1), loc,
                 std::nullopt};
  case '-':
    return Token{TokenType::MINUS, std::string_view(&source[start], 1), loc,
                 std::nullopt};
  case '*':
    return Token{TokenType::STAR, std::string_view(&source[start], 1), loc,
                 std::nullopt};
  case '/':
    return Token{TokenType::SLASH, std::string_view(&source[start], 1), loc,
                 std::nullopt};
  case '=':
    return Token{TokenType::EQUAL, std::string_view(&source[start], 1), loc,
                 std::nullopt};

  default:
    if (is_alpha(c) || c == '_') {
      // Back up and scan identifier
      position--;
      column--;
      return scan_identifier_or_keyword();
    } else if (is_digit(c)) {
      // Back up and scan number
      position--;
      column--;
      return scan_number();
    }

    error(std::string("Unexpected character: '") + c + "'");
    return Token{TokenType::END_OF_FILE, std::string_view{}, loc, std::nullopt};
  }
}

Token Lexer::scan_identifier_or_keyword() {
  SourceLocation loc = current_location();
  std::size_t start = position;

  while (!is_at_end() && is_alnum(peek())) {
    advance();
  }

  std::size_t length = position - start;
  std::string_view text(&source[start], length);

  // Check for keywords
  static const std::unordered_map<std::string_view, TokenType> keywords = {
      {"fun", TokenType::FUN},
      {"return", TokenType::RETURN},
      {"let", TokenType::LET},
      {"i32", TokenType::I32}};

  auto it = keywords.find(text);
  if (it != keywords.end()) {
    return Token{it->second, text, loc, std::nullopt};
  }

  return Token{TokenType::IDENTIFIER, text, loc, std::nullopt};
}

Token Lexer::scan_number() {
  SourceLocation loc = current_location();
  std::size_t start = position;

  while (!is_at_end() && is_digit(peek())) {
    advance();
  }

  std::size_t length = position - start;
  std::string_view text(&source[start], length);

  std::int64_t value = 0;
  for (char c : text) {
    value = value * 10 + (c - '0');
  }

  return Token{TokenType::INTEGER, text, loc, value};
}

SourceLocation Lexer::current_location() const {
  return SourceLocation{filename, line, column};
}

void Lexer::error(const std::string &message) {
  errors.push_back(LexerError{current_location(), message});
}

bool Lexer::is_alpha(char c) const {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool Lexer::is_digit(char c) const { return c >= '0' && c <= '9'; }

bool Lexer::is_alnum(char c) const { return is_alpha(c) || is_digit(c); }