#pragma once

#include <arc/token.hpp>

enum class RaccoonToken
{
  // Keywords
  Fun,
  Let,
  Const,
  Struct,
  Enum,
  Case,
  Match,
  Return,
  If,
  Else,
  While,
  For,
  Break,
  Continue,
  Import,
  Export,
  True,
  False,
  Unique,
  Shared,

  // Type Keywords
  I8,
  I16,
  I32,
  I64,
  U8,
  U16,
  U32,
  U64,
  F32,
  F64,
  Bool,
  Void,
  Usize,

  // Symbols (arithmetic)
  Plus, // +
  Minus, // -
  Star, // *
  Slash, // /
  Percent, // %

  // Symbols (comparison)
  EqualEqual, // ==
  BangEqual, // !=
  Less, // <
  LessEqual, // <=
  Greater, // >
  GreaterEqual, // >=

  // Symbols (logical)
  AndAnd, // &&
  OrOr, // ||
  Bang, // !

  // Symbols (assignment)
  Equal, // =
  PlusEqual, // +=
  MinusEqual, // -=
  StarEqual, // *=
  SlashEqual, // /=
  PercentEqual, // %=

  // Symbols (punctuation)
  LeftParen, // (
  RightParen, // )
  LeftBrace, // {
  RightBrace, // }
  LeftBracket, // [
  RightBracket, // ]
  Colon, // :
  DoubleColon, // ::
  Semicolon, // ;
  Comma, // ,
  Dot, // .
  Arrow, // ->
  FatArrow, // =>
  Question, // ?
  At, // @
  Ampersand, // &
  Caret, // ^
  Tilde, // ~

  // Literals
  Integer,
  Float,
  String,
  Char,

  // Special
  Identifier,
  Eof,
  Error,
};

template<>
struct arc::TokenTraits<RaccoonToken>
{
  using enum RaccoonToken;

  static constexpr auto keywords = arc::keywords<RaccoonToken>("fun", Fun, "let", Let, "const", Const, "struct", Struct,
      "enum", Enum, "case", Case, "match", Match, "return", Return, "if", If, "else", Else, "while", While, "for", For,
      "break", Break, "continue", Continue, "import", Import, "export", Export, "true", True, "false", False, "unique",
      Unique, "shared", Shared, "i8", I8, "i16", I16, "i32", I32, "i64", I64, "u8", U8, "u16", U16, "u32", U32, "u64",
      U64, "f32", F32, "f64", F64, "bool", Bool, "void", Void, "usize", Usize);

  static constexpr auto symbols = arc::symbols<RaccoonToken>(
      // Multi-char
      "==", EqualEqual, "!=", BangEqual, "<=", LessEqual, ">=", GreaterEqual, "&&", AndAnd, "||", OrOr, "+=", PlusEqual,
      "-=", MinusEqual, "*=", StarEqual, "/=", SlashEqual, "%=", PercentEqual, "::", DoubleColon, "->", Arrow, "=>",
      FatArrow,
      // Single char
      "+", Plus, "-", Minus, "*", Star, "/", Slash, "%", Percent, "<", Less, ">", Greater, "!", Bang, "=", Equal, "(",
      LeftParen, ")", RightParen, "{", LeftBrace, "}", RightBrace, "[", LeftBracket, "]", RightBracket, ":", Colon, ";",
      Semicolon, ",", Comma, ".", Dot, "?", Question, "@", At, "&", Ampersand, "^", Caret, "~", Tilde);

  static constexpr arc::LiteralTokens<RaccoonToken> literals {
    .integer = Integer,
    .floating = Float,
    .boolean = std::nullopt, // true/false are keywords
    .string = String,
    .character = Char,
    .identifier = Identifier,
    .eof = Eof,
    .error = Error,
  };
};


constexpr std::string_view raccoon_token_name(RaccoonToken type)
{
  using enum RaccoonToken;
  switch (type)
  {
    case Fun:
      return "fun";
    case Let:
      return "let";
    case Const:
      return "const";
    case Struct:
      return "struct";
    case Enum:
      return "enum";
    case Case:
      return "case";
    case Match:
      return "match";
    case Return:
      return "return";
    case If:
      return "if";
    case Else:
      return "else";
    case While:
      return "while";
    case For:
      return "for";
    case Break:
      return "break";
    case Continue:
      return "continue";
    case Import:
      return "import";
    case Export:
      return "export";
    case True:
      return "true";
    case False:
      return "false";
    case Unique:
      return "unique";
    case Shared:
      return "shared";
    case I8:
      return "i8";
    case I16:
      return "i16";
    case I32:
      return "i32";
    case I64:
      return "i64";
    case U8:
      return "u8";
    case U16:
      return "u16";
    case U32:
      return "u32";
    case U64:
      return "u64";
    case F32:
      return "f32";
    case F64:
      return "f64";
    case Bool:
      return "bool";
    case Void:
      return "void";
    case Usize:
      return "usize";
    case Plus:
      return "+";
    case Minus:
      return "-";
    case Star:
      return "*";
    case Slash:
      return "/";
    case Percent:
      return "%";
    case EqualEqual:
      return "==";
    case BangEqual:
      return "!=";
    case Less:
      return "<";
    case LessEqual:
      return "<=";
    case Greater:
      return ">";
    case GreaterEqual:
      return ">=";
    case AndAnd:
      return "&&";
    case OrOr:
      return "||";
    case Bang:
      return "!";
    case Equal:
      return "=";
    case PlusEqual:
      return "+=";
    case MinusEqual:
      return "-=";
    case StarEqual:
      return "*=";
    case SlashEqual:
      return "/=";
    case PercentEqual:
      return "%=";
    case LeftParen:
      return "(";
    case RightParen:
      return ")";
    case LeftBrace:
      return "{";
    case RightBrace:
      return "}";
    case LeftBracket:
      return "[";
    case RightBracket:
      return "]";
    case Colon:
      return ":";
    case DoubleColon:
      return "::";
    case Semicolon:
      return ";";
    case Comma:
      return ",";
    case Dot:
      return ".";
    case Arrow:
      return "->";
    case FatArrow:
      return "=>";
    case Question:
      return "?";
    case At:
      return "@";
    case Ampersand:
      return "&";
    case Caret:
      return "^";
    case Tilde:
      return "~";
    case Integer:
      return "<integer>";
    case Float:
      return "<float>";
    case String:
      return "<string>";
    case Char:
      return "<char>";
    case Identifier:
      return "<identifier>";
    case Eof:
      return "<eof>";
    case Error:
      return "<error>";
  }
  return "<unknown>";
}
