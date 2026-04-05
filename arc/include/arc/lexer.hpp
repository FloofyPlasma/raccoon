#pragma once

#include "arc/core.hpp"
#include "arc/token.hpp"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstddef>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

namespace arc
{
  struct LexerConfig
  {
    std::vector<std::string> line_comments = { "//" };
    std::vector<std::pair<std::string, std::string>> block_comments = { { "/*", "*/" } };

    bool hex_literals = true; // 0xFF
    bool binary_literals = true; // 0b1010
    bool octal_literals = true; // 0o77

    bool float_suffixes = true; // 3.14f
    bool scientific_notation = true; // 3.14e10, 3.14e-10

    bool escape_sequences = true; // \n \t \\ \"

    std::size_t max_errors = 10;
  };

  template<typename TokenType>
  struct LexResult
  {
    std::vector<Token<TokenType>> tokens;
    std::vector<Diagnostic> diagnostics;

    bool has_errors() const
    {
      return std::ranges::any_of(diagnostics, [](const Diagnostic &d) { return d.is_error(); });
    }

    bool has_warnings() const
    {
      return std::ranges::any_of(diagnostics, [](const Diagnostic &d) { return d.is_warning(); });
    }

    auto begin() { return tokens.begin(); }
    auto end() { return tokens.end(); }
    auto begin() const { return tokens.begin(); }
    auto end() const { return tokens.end(); }

    auto valid() const
    {
      return tokens | std::views::filter([](const Token<TokenType> &t) { return !t.is_error(); });
    }
  };

  template<typename TokenType>
  class Lexer
  {
  public:
    using Traits = TokenTraits<TokenType>;

    Lexer(std::string source, std::string filename, LexerConfig config = { }) :
        source_(std::move(source)), filename_(std::move(filename)), config_(std::move(config))
    {
      build_sorted_symbols();
    }

    LexResult<TokenType> lex()
    {
      LexResult<TokenType> result;

      while (true)
      {
        auto tok = next();
        bool is_eof = tok.is_eof();
        result.tokens.push_back(std::move(tok));
        if (is_eof)
          break;
      }

      result.diagnostics = diagnostics_;
      return result;
    }

    Token<TokenType> next()
    {
      if (peeked_)
      {
        auto tok = std::move(*peeked_);
        peeked_.reset();
        return tok;
      }

      skip_whitespace_and_comments();

      if (is_at_end() || diagnostics_.size() >= config_.max_errors)
      {
        return make_token(Traits::literals.eof, pos_);
      }

      return scan_token();
    }

    const Token<TokenType> &peek()
    {
      if (!peeked_)
      {
        peeked_ = next();
      }
      return *peeked_;
    }

    bool is_at_end() const { return pos_ >= source_.size(); }

    SourceLocation location() const { return { filename_, line_, column_ }; }

    const std::vector<Diagnostic> &diagnostics() const { return diagnostics_; }
    const Diagnostic &last_diagnostic() const { return diagnostics_.back(); }

    bool has_errors() const
    {
      return std::ranges::any_of(diagnostics_, [](const Diagnostic &d) { return d.is_error(); });
    }

  private:
    std::string source_;
    std::string filename_;
    LexerConfig config_;

    std::size_t pos_ = 0;
    uint32_t line_ = 1;
    uint32_t column_ = 1;

    std::vector<Diagnostic> diagnostics_;
    std::optional<Token<TokenType>> peeked_;

    struct SymbolEntry
    {
      std::string_view text;
      TokenType token;
    };
    std::vector<SymbolEntry> sorted_symbols_;

    void build_sorted_symbols()
    {
      sorted_symbols_.reserve(Traits::symbols.size());
      for (const auto &sym: Traits::symbols)
      {
        sorted_symbols_.push_back({ sym.text, sym.token });
      }
      std::sort(sorted_symbols_.begin(), sorted_symbols_.end(),
          [](const SymbolEntry &a, const SymbolEntry &b) { return a.text.size() > b.text.size(); });
    }

    char peek_char(std::size_t offset = 0) const
    {
      if (pos_ + offset >= source_.size())
        return '\0';
      return source_[pos_ + offset];
    }

    char advance_char()
    {
      char c = source_[pos_++];
      if (c == '\n')
      {
        line_++;
        column_ = 1;
      } else
      {
        column_++;
      }
      return c;
    }

    bool match_char(char expected)
    {
      if (peek_char() != expected)
        return false;
      advance_char();
      return true;
    }

    static bool is_ident_start(char c) { return std::isalpha(static_cast<unsigned char>(c)) || c == '_'; }

    static bool is_ident_cont(char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; }

    static bool is_digit(char c) { return std::isdigit(static_cast<unsigned char>(c)); }

    static bool is_hex_digit(char c) { return std::isxdigit(static_cast<unsigned char>(c)); }

    static bool is_binary_digit(char c) { return c == '0' || c == '1'; }
    static bool is_octal_digit(char c) { return c >= '0' && c <= '7'; }

    void skip_whitespace_and_comments()
    {
      while (!is_at_end())
      {
        // Whitespace
        if (std::isspace(static_cast<unsigned char>(peek_char())))
        {
          advance_char();
          continue;
        }

        bool skipped = false;

        // Line comments
        for (const auto &marker: config_.line_comments)
        {
          if (source_.compare(pos_, marker.size(), marker) == 0)
          {
            while (!is_at_end() && peek_char() != '\n')
            {
              advance_char();
            }
            skipped = true;
            break;
          }
        }
        if (skipped)
          continue;

        // Block comments
        for (const auto &[open, close]: config_.block_comments)
        {
          if (source_.compare(pos_, open.size(), open) == 0)
          {
            for (std::size_t i = 0; i < open.size(); i++)
              advance_char();

            while (!is_at_end())
            {
              if (source_.compare(pos_, close.size(), close) == 0)
              {
                for (std::size_t i = 0; i < close.size(); i++)
                  advance_char();
                break;
              }
              advance_char();
            }
            skipped = true;
            break;
          }
        }
        if (skipped)
          continue;

        break;
      }
    }

    Token<TokenType> make_token(TokenType type, std::size_t start, LiteralValue value = LiteralValue::make_none()) const
    {
      std::string_view lexeme { source_.data() + start, pos_ - start };
      return Token<TokenType> { type, lexeme, SourceLocation { filename_, line_, column_ }, std::move(value) };
    }

    Token<TokenType> make_error_token(std::size_t start, std::string message)
    {
      emit_error(start, std::move(message));
      std::string_view lexeme { source_.data() + start, pos_ - start };
      return Token<TokenType> { Traits::literals.error, lexeme, SourceLocation { filename_, line_, column_ },
        LiteralValue::make_none() };
    }

    void emit_error([[maybe_unused]] std::size_t start, std::string message)
    {
      diagnostics_.push_back(
          Diagnostic { Diagnostic::Severity::Error, SourceLocation { filename_, line_, column_ }, std::move(message) });
    }

    Token<TokenType> scan_token()
    {
      std::size_t start = pos_;

      if (peek_char() == '"' && Traits::literals.string.has_value())
      {
        return scan_string(start);
      }

      if (peek_char() == '\'' && Traits::literals.character.has_value())
      {
        return scan_char(start);
      }

      if (is_digit(peek_char()))
      {
        return scan_number(start);
      }

      if (is_ident_start(peek_char()))
      {
        return scan_identifier_or_keyword(start);
      }

      for (const auto &sym: sorted_symbols_)
      {
        if (source_.compare(pos_, sym.text.size(), sym.text) == 0)
        {
          for (std::size_t i = 0; i < sym.text.size(); i++)
            advance_char();
          return make_token(sym.token, start);
        }
      }

      char bad = advance_char();
      std::string msg = std::format("unexpected character '{}'", bad);
      return make_error_token(start, std::move(msg));
    }

    Token<TokenType> scan_identifier_or_keyword(std::size_t start)
    {
      while (!is_at_end() && is_ident_cont(peek_char()))
      {
        advance_char();
      }

      std::string_view text { source_.data() + start, pos_ - start };

      for (const auto &kw: Traits::keywords)
      {
        if (kw.text == text)
        {
          if (Traits::literals.boolean.has_value())
          {
            if (*Traits::literals.boolean == kw.token)
            {
              bool val = (text == "true");
              return make_token(kw.token, start, LiteralValue::make_bool(val));
            }
          }
          return make_token(kw.token, start);
        }
      }

      return make_token(Traits::literals.identifier, start);
    }

    Token<TokenType> scan_number(std::size_t start)
    {
      // Check for 0x / 0b / 0o prefixes
      if (peek_char() == '0')
      {
        char next = peek_char(1);

        if (config_.hex_literals && (next == 'x' || next == 'X'))
        {
          advance_char();
          advance_char(); // consume "0x"
          if (!is_hex_digit(peek_char()))
          {
            return make_error_token(start, "expected hex digits after '0x'");
          }
          while (!is_at_end() && (is_hex_digit(peek_char()) || peek_char() == '_'))
          {
            advance_char();
          }
          std::string_view text { source_.data() + start, pos_ - start };
          std::int64_t val = std::stoll(std::string(text.substr(2)), nullptr, 16);
          return make_token(*Traits::literals.integer, start, LiteralValue::make_integer(val));
        }

        if (config_.binary_literals && (next == 'b' || next == 'B'))
        {
          advance_char();
          advance_char(); // consume "0b"
          if (!is_binary_digit(peek_char()))
          {
            return make_error_token(start, "expected binary digits after '0b'");
          }
          while (!is_at_end() && (is_binary_digit(peek_char()) || peek_char() == '_'))
          {
            advance_char();
          }
          std::string_view text { source_.data() + start, pos_ - start };
          std::int64_t val = std::stoll(std::string(text.substr(2)), nullptr, 2);
          return make_token(*Traits::literals.integer, start, LiteralValue::make_integer(val));
        }

        if (config_.octal_literals && (next == 'o' || next == 'O'))
        {
          advance_char();
          advance_char(); // consume "0o"
          if (!is_octal_digit(peek_char()))
          {
            return make_error_token(start, "expected octal digits after '0o'");
          }
          while (!is_at_end() && (is_octal_digit(peek_char()) || peek_char() == '_'))
          {
            advance_char();
          }
          std::string_view text { source_.data() + start, pos_ - start };
          std::int64_t val = std::stoll(std::string(text.substr(2)), nullptr, 8);
          return make_token(*Traits::literals.integer, start, LiteralValue::make_integer(val));
        }
      }

      while (!is_at_end() && (is_digit(peek_char()) || peek_char() == '_'))
      {
        advance_char();
      }

      bool is_float = false;

      if (peek_char() == '.' && is_digit(peek_char(1)))
      {
        is_float = true;
        advance_char(); // consume '.'
        while (!is_at_end() && (is_digit(peek_char()) || peek_char() == '_'))
        {
          advance_char();
        }
      }

      if (config_.scientific_notation && (peek_char() == 'e' || peek_char() == 'E'))
      {
        is_float = true;
        advance_char(); // consume 'e'
        if (peek_char() == '+' || peek_char() == '-')
          advance_char();
        if (!is_digit(peek_char()))
        {
          return make_error_token(start, "expected digits in exponent");
        }
        while (!is_at_end() && is_digit(peek_char()))
          advance_char();
      }

      bool has_float_suffix = false;
      if (config_.float_suffixes && peek_char() == 'f')
      {
        is_float = true;
        has_float_suffix = true;
        advance_char();
      }

      std::string_view text { source_.data() + start, pos_ - start };

      if (is_float)
      {
        if (!Traits::literals.floating.has_value())
        {
          return make_error_token(start, "float literals not supported in this language");
        }
        std::string clean;
        for (char c: text)
        {
          if (c != '_' && c != 'f')
            clean += c;
        }
        double val = std::stod(clean);
        return make_token(*Traits::literals.floating, start, LiteralValue::make_float(val));
      }

      if (!Traits::literals.integer.has_value())
      {
        return make_error_token(start, "integer literals not supported in this language");
      }

      std::string clean;
      for (char c: text)
      {
        if (c != '_')
          clean += c;
      }
      std::int64_t val = std::stoll(clean);
      return make_token(*Traits::literals.integer, start, LiteralValue::make_integer(val));
    }

    Token<TokenType> scan_string(std::size_t start)
    {
      advance_char(); // consume opening '"'
      std::string value;

      while (!is_at_end() && peek_char() != '"')
      {
        if (peek_char() == '\n')
        {
          return make_error_token(start, "unterminated string literal");
        }

        if (config_.escape_sequences && peek_char() == '\\')
        {
          advance_char(); // consume '\'
          char esc = advance_char();
          switch (esc)
          {
            case 'n':
              value += '\n';
              break;
            case 't':
              value += '\t';
              break;
            case 'r':
              value += '\r';
              break;
            case '\\':
              value += '\\';
              break;
            case '"':
              value += '"';
              break;
            case '\'':
              value += '\'';
              break;
            case '0':
              value += '\0';
              break;
            default:
              emit_error(start, std::format("unknown escape sequence '\\{}'", esc));
              value += esc;
              break;
          }
        } else
        {
          value += advance_char();
        }
      }

      if (is_at_end())
      {
        return make_error_token(start, "unterminated string literal");
      }

      advance_char(); // consume closing '"'
      return make_token(*Traits::literals.string, start, LiteralValue::make_string(std::move(value)));
    }

    Token<TokenType> scan_char(std::size_t start)
    {
      advance_char(); // consume opening '\''

      if (is_at_end())
      {
        return make_error_token(start, "unterminated char literal");
      }

      char value;

      if (config_.escape_sequences && peek_char() == '\\')
      {
        advance_char(); // consume '\'
        char esc = advance_char();
        switch (esc)
        {
          case 'n':
            value = '\n';
            break;
          case 't':
            value = '\t';
            break;
          case 'r':
            value = '\r';
            break;
          case '\\':
            value = '\\';
            break;
          case '\'':
            value = '\'';
            break;
          case '"':
            value = '"';
            break;
          case '0':
            value = '\0';
            break;
          default:
            emit_error(start, std::string("unknown escape sequence '\\") + esc + "'");
            value = esc;
            break;
        }
      } else
      {
        value = advance_char();
      }

      if (!match_char('\''))
      {
        return make_error_token(start, "unterminated char literal");
      }

      return make_token(*Traits::literals.character, start, LiteralValue::make_char(value));
    }
  };

} // namespace arc
