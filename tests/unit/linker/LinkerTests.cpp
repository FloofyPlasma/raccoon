#include <catch2/catch_test_macros.hpp>

#include "raccoon/Linker.hpp"

TEST_CASE("Linker handles errors", "[linker]") {
  SECTION("no object files") {
    Linker linker;
    bool success = linker.link(std::vector<std::string>{}, "output");
    REQUIRE_FALSE(success);
    REQUIRE(linker.has_errors());
  }
}
