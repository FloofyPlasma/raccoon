#include "raccoon/Parser.hpp"

#include <print>

Parser::Parser(std::vector<Token> tokens) : tokens(std::move(tokens)) {}

std::expected<std::unique_ptr<ProgramNode>, std::vector<ParserError>>
Parser::parse() {
  auto program = std::make_unique<ProgramNode>();
  errors.clear();

  while (!is_at_end() && errors.size() < MAX_ERRORS) {
    try {
      if (auto func = parse_function()) {
        program->functions.push_back(std::move(func));
      }
    } catch (...) {
      // Error already recorded, try to re-synchronize
      synchronize();
    }
  }

  if (!errors.empty()) {
    return std::unexpected(std::move(errors));
  }

  return program;
}

#pragma mark Utilities

bool Parser::is_at_end() const { return peek().type == TokenType::END_OF_FILE; }

const Token &Parser::peek() const { return tokens[current]; }

const Token &Parser::previous() const { return tokens[current - 1]; }

const Token &Parser::advance() {
  if (!is_at_end()) {
    current++;
  }

  return previous();
}

bool Parser::check(TokenType type)const {
  if (is_at_end()) {return false;}

  return peek().type == type;
}

bool Parser::match(TokenType type) {
  if (check(type)) {
    advance();

    return true;
  }

  return false;
}

void Parser::error(const std::string &message) {
  errors.push_back(ParserError{peek().location, message});
}

void Parser::synchronize() {
  advance();

  while (!is_at_end()) {
    // If we just passed a semicolon, assume we're synchronized
    if (previous().type == TokenType::SEMICOLON) {
      return;
    }

    // Stop at statement/declaration boundaries
    switch (peek().type) {
    case TokenType::FUN:
    case TokenType::RETURN:
      return;
    default:
      advance();
    }
  }
}

bool Parser::expect(TokenType type, const std::string &message) {
  if (check(type)) {
    advance();

    return true;
  }

  error(message);

  return false;
}

#pragma mark Parsing functions

std::unique_ptr<FunctionDecl> Parser::parse_function() {
  SourceLocation loc = peek().location;

  if (!expect(TokenType::FUN, "Expected 'fun'")) {
    return nullptr;
  }

  if (!check(TokenType::IDENTIFIER)) {
    error("Expected function name");
    return nullptr;
  }

  std::string name(advance().lexeme);
  auto func = std::make_unique<FunctionDecl>(std::move(name), loc);

  if (!expect(TokenType::LEFT_PAREN, "Expected '(' after function name")) {
    return nullptr;
  }

  // TODO: Param parsing

  if (!expect(TokenType::RIGHT_PAREN, "Expected ')'")) {
    return nullptr;
  }

  if (!expect(TokenType::COLON, "Expected ':' before return type")) {
    return nullptr;
  }

  func->return_type = parse_type();
  if (!func->return_type) {
    return nullptr;
  }

  func->body = parse_block();
  if (!func->body) {
    return nullptr;
  }

  return func;
}

std::unique_ptr<Type> Parser::parse_type() {
  SourceLocation loc = peek().location;

  if (match(TokenType::I32)) {
    return std::make_unique<Type>(Type::Kind::I32, loc);
  }

  error("Expected type");
  return nullptr;
}

std::unique_ptr<BlockStmt> Parser::parse_block() {
  SourceLocation loc = peek().location;

  if (!expect(TokenType::LEFT_BRACE, "Expected '{'")) {
    return nullptr;
  }

  auto block = std::make_unique<BlockStmt>(loc);

  while (!check(TokenType::RIGHT_BRACE) && !is_at_end()) {
    if (auto stmt = parse_statement()) {
      block->statements.push_back(std::move(stmt));
    } else {
      synchronize();
    }

    if (errors.size() >= MAX_ERRORS) {
      break;
    }
  }

  if (!expect(TokenType::RIGHT_BRACE, "Expected '}'")) {
    return nullptr;
  }

  return block;
}

std::unique_ptr<Statement> Parser::parse_statement() {
  SourceLocation loc = peek().location;

  if (match(TokenType::RETURN)) {
    auto expr = parse_expression();
    if (!expr) {
      return nullptr;
    }

    if (!expect(TokenType::SEMICOLON, "Expected ';' after return statement")) {
      return nullptr;
    }

    return std::make_unique<ReturnStmt>(std::move(expr), loc);
  }

  error("Expected statement");

  return nullptr;
}

std::unique_ptr<Expression> Parser::parse_expression() {
  // Only primary for now
  // TODO: Expand
  return parse_primary();
}

std::unique_ptr<Expression> Parser::parse_primary() {
  SourceLocation loc = peek().location;

  if (peek().type == TokenType::INTEGER) {
    int64_t value = peek().int_value.value();
    advance();
    return std::make_unique<IntegerLiteral>(value, loc);
  }

  error("Expected expression");
  return nullptr;
}