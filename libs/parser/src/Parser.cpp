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

bool Parser::check(TokenType type) const {
  if (is_at_end()) {
    return false;
  }

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
    case TokenType::LET:
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

#pragma mark Precedence

int Parser::get_precedence(TokenType type) {
  switch (type) {
  case TokenType::PLUS:
  case TokenType::MINUS:
    return 9;

  case TokenType::STAR:
  case TokenType::SLASH:
    return 10;

  default:
    return 0;
  }
}

bool Parser::is_binary_operator(TokenType type) {
  return get_precedence(type) > 0;
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

  if (check(TokenType::LET)) {
    return parse_var_decl();
  }

  if (match(TokenType::RETURN)) {
    std::unique_ptr<Expression> expr;

    if (!check(TokenType::SEMICOLON)) {
      expr = parse_expression();
      if (!expr) {
        return nullptr;
      }
    }

    if (!expect(TokenType::SEMICOLON, "Expected ';' after return statement")) {
      return nullptr;
    }

    return std::make_unique<ReturnStmt>(std::move(expr), loc);
  }

  if (check(TokenType::IDENTIFIER) && current + 1 < tokens.size() &&
      tokens[current + 1].type == TokenType::EQUAL) {
    std::string name(advance().lexeme);
    advance();

    auto value = parse_expression();
    if (!value) {
      return nullptr;
    }

    if (!expect(TokenType::SEMICOLON, "Expected ';' after assignment")) {
      return nullptr;
    }

    return std::make_unique<AssignmentStmt>(std::move(name), std::move(value),
                                            loc);
  }

  auto expr = parse_expression();
  if (!expr) {
    error("Expected statement");
    return nullptr;
  }

  if (!expect(TokenType::SEMICOLON, "Expected ';' after expression")) {
    return nullptr;
  }

  return std::make_unique<ExpressionStmt>(std::move(expr), loc);
}

std::unique_ptr<Statement> Parser::parse_var_decl() {
  SourceLocation loc = peek().location;

  if (!expect(TokenType::LET, "Expected 'let'")) {
    return nullptr;
  }

  if (!check(TokenType::IDENTIFIER)) {
    error("Expected variable name after 'let'");
    return nullptr;
  }

  std::string name(advance().lexeme);

  if (!expect(TokenType::COLON, "Expected ':' after variable name")) {
    return nullptr;
  }

  auto type = parse_type();
  if (!type) {
    return nullptr;
  }

  std::unique_ptr<Expression> initializer;
  if (match(TokenType::EQUAL)) {
    initializer = parse_expression();
    if (!initializer) {
      return nullptr;
    }
  }

  if (!expect(TokenType::SEMICOLON,
              "Expected ';' after variable declaration")) {
    return nullptr;
  }

  return std::make_unique<VarDeclStmt>(std::move(name), std::move(type),
                                       std::move(initializer), loc);
}

std::unique_ptr<Expression> Parser::parse_expression(int min_precedence) {
  auto left = parse_primary();
  if (!left) {
    return nullptr;
  }

  while (is_binary_operator(peek().type) &&
         get_precedence(peek().type) >= min_precedence) {
    TokenType op = peek().type;
    SourceLocation op_loc = peek().location;
    advance();

    int next_min_prec = get_precedence(op) + 1;
    auto right = parse_expression(next_min_prec);
    if (!right) {
      return nullptr;
    }

    left = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right),
                                        op_loc);
  }

  return left;
}

std::unique_ptr<Expression> Parser::parse_primary() {
  SourceLocation loc = peek().location;

  // Integer literal
  if (peek().type == TokenType::INTEGER) {
    int64_t value = peek().int_value.value();
    advance();
    return std::make_unique<IntegerLiteral>(value, loc);
  }

  // Identifier
  if (peek().type == TokenType::IDENTIFIER) {
    std::string name(advance().lexeme);
    return std::make_unique<IdentifierExpr>(std::move(name), loc);
  }

  // Parenthesized expression
  if (match(TokenType::LEFT_PAREN)) {
    auto expr = parse_expression();
    if (!expr) {
      return nullptr;
    }
    if (!expect(TokenType::RIGHT_PAREN, "Expected ')' after expression")) {
      return nullptr;
    }
    return expr;
  }

  error("Expected expression");
  return nullptr;
}