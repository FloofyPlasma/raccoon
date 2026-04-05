#pragma once

#include <cstdint>
#include <string>
#include <variant>

namespace arc
{
  struct SourceLocation
  {
    std::string filename;
    std::uint32_t line = 0;
    std::uint32_t column = 0;
  };

  struct Diagnostic
  {
    enum class Severity
    {
      Error,
      Warning,
      Note,
    };

    Severity severity;
    SourceLocation location;
    std::string message;

    bool is_error() const { return severity == Severity::Error; }
    bool is_warning() const { return severity == Severity::Warning; }
    bool is_note() const { return severity == Severity::Note; }
  };

  struct LiteralValue
  {
    using Storage = std::variant<std::monostate, // None
        std::int64_t, // Integer
        double, // Float
        bool, // Bool
        std::string, // String
        char // Char
        >;

    Storage data;

    static LiteralValue make_none() { return LiteralValue { }; }

    static LiteralValue make_integer(std::int64_t v)
    {
      LiteralValue lv;
      lv.data = v;
      return lv;
    }

    static LiteralValue make_float(double v)
    {
      LiteralValue lv;
      lv.data = v;
      return lv;
    }

    static LiteralValue make_bool(bool v)
    {
      LiteralValue lv;
      lv.data = v;
      return lv;
    }

    static LiteralValue make_string(std::string v)
    {
      LiteralValue lv;
      lv.data = v;
      return lv;
    }

    static LiteralValue make_char(char v)
    {
      LiteralValue lv;
      lv.data = v;
      return lv;
    }

    bool is_none() const { return std::holds_alternative<std::monostate>(data); }
    bool is_integer() const { return std::holds_alternative<int64_t>(data); }
    bool is_float() const { return std::holds_alternative<double>(data); }
    bool is_bool() const { return std::holds_alternative<bool>(data); }
    bool is_string() const { return std::holds_alternative<std::string>(data); }
    bool is_char() const { return std::holds_alternative<char>(data); }

    std::int64_t integer() const { return std::get<std::int64_t>(data); }
    double floating() const { return std::get<double>(data); }
    bool boolean() const { return std::get<bool>(data); }
    const std::string &string() const { return std::get<std::string>(data); }
    char character() const { return std::get<char>(data); }
  };
} // namespace arc
