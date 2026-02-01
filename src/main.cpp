#include "raccoon/AST.hpp"
#include "raccoon/ASTPrinter.hpp"
#include "raccoon/Lexer.hpp"
#include "raccoon/Parser.hpp"
#include "raccoon/Token.hpp"
#include <cstring>
#include <fstream>
#include <iostream>
#include <print>
#include <sstream>

struct Option {
  const char *flag;
  const char *arg_name; // empty string if no argument
  const char *description;
  bool *bool_target = nullptr;
  std::string *string_target = nullptr;
};

bool dump_tokens_flag = false;
bool dump_ast_flag = false;
std::string output_file = "a.out";

Option constexpr options[] = {
    {"--dump-tokens", "", "Print tokens and exit", &dump_tokens_flag, nullptr},
    {"--dump-ast", "", "Print AST and exit", &dump_ast_flag, nullptr},
    {"-o", "<file>", "Output file (default: a.out)", nullptr, &output_file},
};

void print_usage(const char *prog_name) {
  std::println("Raccoon Compiler v0.1.0\n");
  std::println("Usage: {} [options] <source.rac>\n", prog_name);
  std::println("Options:");
  for (const auto &opt : options) {
    if (opt.arg_name[0] != '\0') {
      std::println("  {} {}", opt.flag, opt.arg_name);
    } else {
      std::println("  {}", opt.flag);
    }
    std::println("      {}", opt.description);
  }
}

struct ParseResult {
  bool success;
  std::optional<std::string> input_file;
  std::string error_message;
};

ParseResult parse_args(int argc, char *argv[]) {
  ParseResult result{true, std::nullopt, ""};

  for (int i = 1; i < argc; ++i) {
    bool matched = false;

    // Try to match against known options
    for (auto &opt : options) {
      if (std::strcmp(argv[i], opt.flag) == 0) {
        matched = true;

        if (opt.bool_target) {
          // Boolean flag
          *opt.bool_target = true;
        } else if (opt.string_target) {
          // String argument
          if (i + 1 >= argc) {
            result.success = false;
            result.error_message =
                std::format("Option {} requires an argument", opt.flag);
            return result;
          }
          *opt.string_target = argv[++i];
        }
        break;
      }
    }

    if (!matched) {
      // Not a known option - could be input file
      if (argv[i][0] != '-') {
        if (result.input_file.has_value()) {
          // Multiple input files not supported
          result.success = false;
          result.error_message = "Multiple input files specified";
          return result;
        }
        result.input_file = argv[i];
      } else {
        // Unknown option
        result.success = false;
        result.error_message = std::format("Unknown option: {}", argv[i]);
        return result;
      }
    }
  }

  return result;
}

std::string read_file(const std::string &filename) {
  std::ifstream file(filename);
  if (!file) {
    throw std::runtime_error("Could not open file " + filename);
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

void dump_tokens(const std::vector<Token> &tokens) {
  std::println("Tokens:");
  std::println("-------");
  for (const auto &token : tokens) {
    std::print("{}", token_type_to_string(token.type));

    if (!token.lexeme.empty()) {
      std::print(" '{}'", token.lexeme);
    }

    std::print(" at {}:{}:{}", token.location.filename, token.location.line,
               token.location.column);

    if (token.int_value.has_value()) {
      std::print(" (value: {})", token.int_value.value());
    }

    std::println("");
  }
  std::println("\nTotal: {} tokens", tokens.size());
}

void print_lexer_errors(const std::vector<LexerError> &errors) {
  for (const auto &error : errors) {
    std::println(std::cerr, "{}:{}:{}: error: {}", error.location.filename,
                 error.location.line, error.location.column, error.message);
  }
}

void print_parser_errors(const std::vector<ParserError> &errors) {
  for (const auto &error : errors) {
    std::println(std::cerr, "{}:{}:{}: error: {}", error.location.filename,
                 error.location.line, error.location.column, error.message);
  }
}

int main(int argc, char *argv[]) {
  auto parse_result = parse_args(argc, argv);

  if (!parse_result.success) {
    std::println(std::cerr, "Error: {}\n", parse_result.error_message);
    print_usage(argv[0]);
    return 1;
  }

  if (!parse_result.input_file.has_value()) {
    std::println(std::cerr, "Error: No input file specified\n");
    print_usage(argv[0]);
    return 1;
  }

  const std::string &input_file = parse_result.input_file.value();

  try {
    std::string source = read_file(input_file);

    Lexer lexer(source, input_file);
    auto tokens = lexer.lex();

    if (lexer.has_errors()) {
      print_lexer_errors(lexer.get_errors());
      return 1;
    }

    if (dump_tokens_flag) {
      dump_tokens(tokens);
      return 0;
    }

    Parser parser(std::move(tokens));
    auto parser_result = parser.parse();

    if (!parser_result.has_value()) {
      print_parser_errors(parser_result.error());
      return 1;
    }

    auto ast = std::move(parser_result.value());

    if (dump_ast_flag) {
      std::println("AST:");
      std::println("----");
      ASTPrinter printer;
      ast->accept(printer);
      return 0;
    }

    std::println("Parsing successful: {} functions", ast->functions.size());

    return 0;

  } catch (const std::exception &e) {
    std::println(std::cerr, "Error: {}", e.what());
    return 1;
  }
}
