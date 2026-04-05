#pragma once

#include <array>
#include <optional>

#include "arc/core.hpp"

namespace arc
{
  template<typename TokenType>
  struct Keyword
  {
    std::string_view text;
    TokenType token;
  };

  template<typename TokenType>
  struct Symbol
  {
    std::string_view text;
    TokenType token;
  };

  template<typename TokenType>
  struct LiteralTokens
  {
    std::optional<TokenType> integer;
    std::optional<TokenType> floating;
    std::optional<TokenType> boolean;
    std::optional<TokenType> string;
    std::optional<TokenType> character;
    TokenType identifier;
    TokenType eof;
    TokenType error;
  };

  namespace detail
  {
    template<typename TokenType, std::size_t N, typename Tuple, std::size_t... Is>
    constexpr std::array<Keyword<TokenType>, N> make_keywords_impl(Tuple &&t, std::index_sequence<Is...>)
    {
      return { { Keyword<TokenType> {
          std::get<Is * 2>(t),
          std::get<Is * 2 + 1>(t),
      }... } };
    }

    template<typename TokenType, std::size_t N, typename Tuple, std::size_t... Is>
    constexpr std::array<Symbol<TokenType>, N> make_symbols_impl(Tuple &&t, std::index_sequence<Is...>)
    {
      return { { Symbol<TokenType> {
          std::get<Is * 2>(t),
          std::get<Is * 2 + 1>(t),
      }... } };
    }
  } // namespace detail

  template<typename TokenType, typename... Args>
  constexpr auto keywords(Args &&...args)
  {
    static_assert(sizeof...(Args) % 2 == 0, "arc::keywords requires alternating (string_view, TokenType) pairs");
    constexpr std::size_t N = sizeof...(Args) / 2;
    return detail::make_keywords_impl<TokenType, N>(
        std::forward_as_tuple(std::forward<Args>(args)...), std::make_index_sequence<N> { });
  }

  template<typename TokenType, typename... Args>
  constexpr auto symbols(Args &&...args)
  {
    static_assert(sizeof...(Args) % 2 == 0, "arc::symbols requires alternating (string_view, TokenType) pairs");
    constexpr std::size_t N = sizeof...(Args) / 2;
    return detail::make_symbols_impl<TokenType, N>(
        std::forward_as_tuple(std::forward<Args>(args)...), std::make_index_sequence<N> { });
  }

  template<typename TokenType, typename... Args>
  constexpr auto sync_points(Args &&...args)
  {
    return std::array<TokenType, sizeof...(Args)> { { static_cast<TokenType>(args)... } };
  }

  template<typename TokenType>
  struct TokenTraits
  {
    static_assert(sizeof(TokenType) == 0, "You must specialize arc::TokenType<T> for your token type. "
                                          "See arc/token.hpp for the required interface");
  };

  template<typename TokenType>
  struct Token
  {
    TokenType type;
    std::string_view lexeme; // Non-owning, source must outlive tokens
    SourceLocation location;
    LiteralValue value;

    bool is(TokenType t) const { return type == t; }
    bool has_value() const { return !value.is_none(); }

    bool is_eof() const { return type == TokenTraits<TokenType>::literals.eof; }

    bool is_error() const { return type == TokenTraits<TokenType>::literals.error; }

    std::string lexeme_owned() const { return std::string(lexeme); }
  };
} // namespace arc
