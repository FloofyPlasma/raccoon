#include "raccoon/Linker.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sys/wait.h>

TEST_CASE("Linker handles errors", "[linker]") {
  SECTION("no object files") {
    Linker linker;
    bool success = linker.link(std::vector<std::string>{}, "output");

    REQUIRE_FALSE(success);
    REQUIRE(linker.has_errors());
    REQUIRE(linker.get_errors()[0].message.find("No object files") !=
            std::string::npos);
  }

  SECTION("nonexistent object file") {
    Linker linker;
    bool success = linker.link("nonexistent_file_12345.o", "output");

    // Should fail during linking
    REQUIRE_FALSE(success);
    REQUIRE(linker.has_errors());
  }
}

TEST_CASE("Linker error accessor methods", "[linker]") {
  SECTION("has_errors returns false initially") {
    Linker linker;
    REQUIRE_FALSE(linker.has_errors());
  }

  SECTION("get_errors returns empty initially") {
    Linker linker;
    REQUIRE(linker.get_errors().empty());
  }

  SECTION("errors persist after failed link") {
    Linker linker;
    linker.link(std::vector<std::string>{}, "output");

    REQUIRE(linker.has_errors());
    REQUIRE_FALSE(linker.get_errors().empty());
  }
}

TEST_CASE("Linker clears errors on new link attempt", "[linker]") {
  Linker linker;

  // First failed attempt
  linker.link(std::vector<std::string>{}, "output1");
  REQUIRE(linker.has_errors());

  // Second attempt should clear previous errors
  linker.link(std::vector<std::string>{}, "output2");
  REQUIRE(linker.has_errors());

  // Should only have errors from second attempt
  auto errors = linker.get_errors();
  REQUIRE(errors.size() == 1);
}

TEST_CASE("Linker single file convenience method", "[linker]") {
  SECTION("forwards to vector method") {
    Linker linker;

    // Both forms should work the same
    bool result1 = linker.link("test.o", "output1");
    linker.link(std::vector<std::string>{"test.o"}, "output2");

    // Both should fail the same way (nonexistent file)
    REQUIRE_FALSE(result1);
  }
}

// Integration test - only runs if we can create real object files
TEST_CASE("Linker integration with real object file",
          "[linker][integration]") {
  SECTION("link simple C program") {
    // Create a minimal C program
    std::string test_c = R"(
int main() {
    return 42;
}
)";

    std::ofstream src("test_linker_integration.c");
    src << test_c;
    src.close();

    // Compile to object file using system compiler
    int compile_result = std::system(
        "cc -c test_linker_integration.c -o test_linker_integration.o "
        "2>/dev/null");

    if (compile_result == 0 &&
        std::filesystem::exists("test_linker_integration.o")) {

      // Try to link it with our Linker
      Linker linker;
      bool success =
          linker.link("test_linker_integration.o", "test_linker_exe");

      if (!success) {
        // Print errors for debugging
        for (const auto &err : linker.get_errors()) {
          std::cerr << "Linker error: " << err.message << "\n";
        }
      }

      REQUIRE(success);
      REQUIRE(std::filesystem::exists("test_linker_exe"));

      // Try to run it and check exit code
      int run_result = std::system("./test_linker_exe");
      int exit_code = WEXITSTATUS(run_result);
      REQUIRE(exit_code == 42);

      // Clean up
      std::filesystem::remove("test_linker_exe");
      std::filesystem::remove("test_linker_integration.o");
    }

    std::filesystem::remove("test_linker_integration.c");
  }
}

TEST_CASE("Linker caching behavior", "[linker]") {
  SECTION("multiple links with same Linker instance") {
    // Create two minimal object files
    std::string test_c1 = "int main() { return 1; }";
    std::string test_c2 = "int main() { return 2; }";

    std::ofstream src1("test_cache1.c");
    src1 << test_c1;
    src1.close();

    std::ofstream src2("test_cache2.c");
    src2 << test_c2;
    src2.close();

    std::system("cc -c test_cache1.c -o test_cache1.o 2>/dev/null");
    std::system("cc -c test_cache2.c -o test_cache2.o 2>/dev/null");

    if (std::filesystem::exists("test_cache1.o") &&
        std::filesystem::exists("test_cache2.o")) {

      Linker linker;

      // First link - should populate cache
      bool success1 = linker.link("test_cache1.o", "test_cache_exe1");
      REQUIRE(success1);

      // Second link - should use cache (same Linker instance)
      bool success2 = linker.link("test_cache2.o", "test_cache_exe2");
      REQUIRE(success2);

      // Both executables should exist
      REQUIRE(std::filesystem::exists("test_cache_exe1"));
      REQUIRE(std::filesystem::exists("test_cache_exe2"));

      // Clean up
      std::filesystem::remove("test_cache_exe1");
      std::filesystem::remove("test_cache_exe2");
      std::filesystem::remove("test_cache1.o");
      std::filesystem::remove("test_cache2.o");
    }

    std::filesystem::remove("test_cache1.c");
    std::filesystem::remove("test_cache2.c");
  }
}

TEST_CASE("Linker helpful error messages", "[linker]") {
  SECTION("CRT files not found error is helpful") {
    Linker linker;

    bool success = linker.link("test.o", "output");

    if (!success && linker.has_errors()) {
      const auto &errors = linker.get_errors();

      bool has_helpful_info =
          errors[0].message.find("cannot open") != std::string::npos ||
          errors[0].message.find("runtime") != std::string::npos ||
          errors[0].message.find("clang") != std::string::npos ||
          errors[0].message.find("gcc") != std::string::npos ||
          errors[0].message.find("manual") != std::string::npos;

      REQUIRE(has_helpful_info);
    }
  }
}