#include <catch2/catch_test_macros.hpp>

#include <dotenv.hpp>

using namespace dotenv;

TEST_CASE("smoke: parse dotenv content", "[smoke]") {
    auto vars = parse_dotenv("FOO=bar\nBAZ=qux\n");
    REQUIRE(vars.size() == 2);
    CHECK(vars[0].key == "FOO");
    CHECK(vars[0].value == "bar");
    CHECK(vars[1].key == "BAZ");
    CHECK(vars[1].value == "qux");
}

TEST_CASE("smoke: merge last write wins", "[smoke]") {
    auto vars = merge({{"A", "1"}, {"A", "2"}});
    REQUIRE(vars.size() == 1);
    CHECK(vars[0].value == "2");
}

TEST_CASE("smoke: expand variable references", "[smoke]") {
    std::vector<EnvironmentVariable> vars = {
        {"HOST", "localhost"},
        {"URL", "http://${HOST}:8080"},
    };
    expand(vars);
    CHECK(vars[1].value == "http://localhost:8080");
}
