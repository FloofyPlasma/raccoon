#include <fstream>
#include <print>
#include <sstream>

#include "RaccoonLexer.hpp"

struct Options
{
  std::string input_file;
  bool dump_tokens = false;
  bool help = false;
};

void print_usage(const char *prog)
{
  std::println("Raccoon Compiler v0.1.0\n");
  std::println("Usage: {} [options] <source.rac>\n", prog);
  std::println("Options:");
  std::println("\t--dump-tokens Print tokens and exit");
  std::println("\t--help        Shows this help");
}

std::optional<Options> parse_args(int argc, char *argv[])
{
  Options opts;

  for (int i = 1; i < argc; i++)
  {
    std::string_view arg = argv[i];

    if (arg == "--help" || arg == "-h")
    {
      opts.help = true;
      return opts;
    } else if (arg == "--dump-tokens")
    {
      opts.dump_tokens = true;
    } else if (arg.starts_with('-'))
    {
      std::println(stderr, "error: unknown option '{}'\n", arg);
      return std::nullopt;
    } else
    {
      if (!opts.input_file.empty())
      {
        std::println(stderr, "error: multiple input files specified\n");
        return std::nullopt;
      }
      opts.input_file = arg;
    }
  }

  return opts;
}

std::optional<std::string> read_file(const std::string &path)
{
  std::ifstream file(path);
  if (!file)
  {
    std::println(stderr, "error: could not open '{}'", path);
    return std::nullopt;
  }
  std::ostringstream buf;
  buf << file.rdbuf();
  return buf.str();
}

void print_diagnostics(const std::vector<arc::Diagnostic> &diagnostics)
{
  for (const auto &diag: diagnostics)
  {
    std::string_view severity = diag.is_error() ? "error" : diag.is_warning() ? "warning" : "note";

    std::println(stderr, "{}:{}:{}: {}: {}\n", diag.location.filename, diag.location.line, diag.location.column,
        severity, diag.message);
  }
}

void dump_tokens(const arc::LexResult<RaccoonToken> &result)
{
  std::println("Tokens:");
  std::println("-------");

  for (const auto &tok: result.tokens)
  {
    std::print(
        "{:<4}:{:<4}  {:<16}  '{}'", tok.location.line, tok.location.column, raccoon_token_name(tok.type), tok.lexeme);

    if (tok.has_value())
    {
      if (tok.value.is_integer())
      {
        std::print("  ({})", tok.value.integer());
      } else if (tok.value.is_float())
      {
        std::print("  ({})", tok.value.floating());
      } else if (tok.value.is_string())
      {
        std::print("  (\"{}\")", tok.value.string());
      } else if (tok.value.is_char())
      {
        std::print("  ('{}')", tok.value.character());
      }
    }

    std::println();
  }

  std::println("\nTotal: {} tokens", result.tokens.size());
}

int main(int argc, char *argv[])
{
  auto opts = parse_args(argc, argv);
  if (!opts)
  {
    print_usage(argv[0]);
    return 1;
  }

  if (opts->help)
  {
    print_usage(argv[0]);
    return 0;
  }

  if (opts->input_file.empty())
  {
    std::println(stderr, "error: no input file specified\n");
    print_usage(argv[0]);
    return 1;
  }

  auto source = read_file(opts->input_file);
  if (!source)
    return 1;

  RaccoonLexer lexer(std::move(*source), opts->input_file);
  auto lex_result = lexer.lex();

  if (!lex_result.diagnostics.empty())
  {
    print_diagnostics(lex_result.diagnostics);
  }

  if (opts->dump_tokens)
  {
    dump_tokens(lex_result);
    return lex_result.has_errors() ? 1 : 0;
  }

  if (lex_result.has_errors())
  {
    return 1;
  }

  return 0;
}
