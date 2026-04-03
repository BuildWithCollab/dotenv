#include <catch2/catch_test_macros.hpp>

#include "expand.hpp"

#include <cstdlib>

using namespace dotenv;
using namespace dotenv::detail;

// ── no-op cases ──────────────────────────────────────────────────────────

TEST_CASE("expand: empty input does nothing", "[expand]") {
    std::vector<EnvironmentVariable> vars;
    expand(vars);
    CHECK(vars.empty());
}

TEST_CASE("expand: no references — values unchanged", "[expand]") {
    std::vector<EnvironmentVariable> vars = {{"FOO", "bar"}, {"BAZ", "qux"}};
    expand(vars);
    CHECK(vars[0].value == "bar");
    CHECK(vars[1].value == "qux");
}

// ── basic ${VAR} expansion ───────────────────────────────────────────────

TEST_CASE("expand: simple ${VAR} reference", "[expand]") {
    std::vector<EnvironmentVariable> vars = {{"HOST", "localhost"}, {"URL", "http://${HOST}"}};
    expand(vars);
    CHECK(vars[1].value == "http://localhost");
}

TEST_CASE("expand: multiple references in one value", "[expand]") {
    std::vector<EnvironmentVariable> vars = {
        {"HOST", "localhost"},
        {"PORT", "5432"},
        {"URL", "postgres://${HOST}:${PORT}/db"},
    };
    expand(vars);
    CHECK(vars[2].value == "postgres://localhost:5432/db");
}

TEST_CASE("expand: reference to later-defined var", "[expand]") {
    std::vector<EnvironmentVariable> vars = {{"URL", "http://${HOST}"}, {"HOST", "example.com"}};
    expand(vars);
    CHECK(vars[0].value == "http://example.com");
}

// ── chained references ───────────────────────────────────────────────────

TEST_CASE("expand: chained references (A→B→C)", "[expand]") {
    std::vector<EnvironmentVariable> vars = {
        {"C", "hello"},
        {"B", "${C}-world"},
        {"A", "${B}!"},
    };
    expand(vars);
    CHECK(vars[2].value == "hello-world!");
}

// ── case-insensitive lookup ──────────────────────────────────────────────

TEST_CASE("expand: case-insensitive variable lookup", "[expand]") {
    std::vector<EnvironmentVariable> vars = {{"host", "localhost"}, {"URL", "http://${HOST}"}};
    expand(vars);
    CHECK(vars[1].value == "http://localhost");
}

// ── fallback to real environment ─────────────────────────────────────────

TEST_CASE("expand: falls back to real environment variable", "[expand]") {
    // PATH should exist on every platform
    std::vector<EnvironmentVariable> vars = {{"HAS_PATH", "${PATH}"}};
    expand(vars);
    // Should be non-empty (PATH is always set)
    CHECK(!vars[0].value.empty());
    CHECK(vars[0].value != "${PATH}");
}

// ── unknown refs preserved ───────────────────────────────────────────────

TEST_CASE("expand: unknown reference preserved as-is", "[expand]") {
    std::vector<EnvironmentVariable> vars = {{"FOO", "prefix-${DOES_NOT_EXIST_QWERTY}-suffix"}};
    expand(vars);
    CHECK(vars[0].value == "prefix-${DOES_NOT_EXIST_QWERTY}-suffix");
}

// ── circular / self-reference protection ─────────────────────────────────

TEST_CASE("expand: self-reference does not infinite loop", "[expand]") {
    std::vector<EnvironmentVariable> vars = {{"FOO", "${FOO}"}};
    expand(vars);  // Should terminate (max iterations)
    // Value is itself — can't resolve, but shouldn't crash
}

TEST_CASE("expand: circular reference does not infinite loop", "[expand]") {
    std::vector<EnvironmentVariable> vars = {{"A", "${B}"}, {"B", "${A}"}};
    expand(vars);  // Should terminate (max iterations)
}

// ── dollar sign without braces ───────────────────────────────────────────

TEST_CASE("expand: bare $ without { is not expanded", "[expand]") {
    std::vector<EnvironmentVariable> vars = {{"PRICE", "$5.00"}};
    expand(vars);
    CHECK(vars[0].value == "$5.00");
}

TEST_CASE("expand: unclosed ${ is not expanded", "[expand]") {
    std::vector<EnvironmentVariable> vars = {{"FOO", "hello ${NOPE"}};
    expand(vars);
    CHECK(vars[0].value == "hello ${NOPE");
}
