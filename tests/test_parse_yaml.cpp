#include <catch2/catch_test_macros.hpp>

#include <dotenv/parse.hpp>

using namespace dotenv;

// ── empty / invalid ──────────────────────────────────────────────────────

TEST_CASE("parse_yaml: empty string returns no vars", "[parse_yaml]") {
    auto vars = parse_yaml("");
    CHECK(vars.empty());
}

TEST_CASE("parse_yaml: non-map root returns no vars", "[parse_yaml]") {
    auto vars = parse_yaml("- item1\n- item2\n");
    CHECK(vars.empty());
}

TEST_CASE("parse_yaml: invalid yaml returns no vars", "[parse_yaml]") {
    auto vars = parse_yaml("{{{{not yaml at all");
    CHECK(vars.empty());
}

// ── basic key: value ─────────────────────────────────────────────────────

TEST_CASE("parse_yaml: simple string values", "[parse_yaml]") {
    auto vars = parse_yaml("FOO: bar\nBAZ: qux\n");
    REQUIRE(vars.size() == 2);
    CHECK(vars[0].key == "FOO");
    CHECK(vars[0].value == "bar");
    CHECK(vars[1].key == "BAZ");
    CHECK(vars[1].value == "qux");
}

TEST_CASE("parse_yaml: numeric values become strings", "[parse_yaml]") {
    auto vars = parse_yaml("PORT: 5432\nRATIO: 3.14\n");
    REQUIRE(vars.size() == 2);
    CHECK(vars[0].key == "PORT");
    CHECK(vars[0].value == "5432");
    CHECK(vars[1].key == "RATIO");
    CHECK(vars[1].value == "3.14");
}

TEST_CASE("parse_yaml: boolean values become strings", "[parse_yaml]") {
    auto vars = parse_yaml("DEBUG: true\nVERBOSE: false\n");
    REQUIRE(vars.size() == 2);
    CHECK(vars[0].key == "DEBUG");
    CHECK(vars[0].value == "true");
    CHECK(vars[1].key == "VERBOSE");
    CHECK(vars[1].value == "false");
}

TEST_CASE("parse_yaml: quoted string value", "[parse_yaml]") {
    auto vars = parse_yaml("SECRET: \"my secret\"\n");
    REQUIRE(vars.size() == 1);
    CHECK(vars[0].key == "SECRET");
    CHECK(vars[0].value == "my secret");
}

// ── empty / null values ──────────────────────────────────────────────────

TEST_CASE("parse_yaml: empty value (key with no value)", "[parse_yaml]") {
    auto vars = parse_yaml("EMPTY:\n");
    REQUIRE(vars.size() == 1);
    CHECK(vars[0].key == "EMPTY");
    CHECK(vars[0].value == "");
}

TEST_CASE("parse_yaml: null value becomes empty string", "[parse_yaml]") {
    auto vars = parse_yaml("GONE: null\n");
    REQUIRE(vars.size() == 1);
    CHECK(vars[0].key == "GONE");
    CHECK(vars[0].value == "");
}

// ── complex values ───────────────────────────────────────────────────────

TEST_CASE("parse_yaml: value containing special chars", "[parse_yaml]") {
    auto vars = parse_yaml("URL: \"postgres://host:5432/db?opt=1\"\n");
    REQUIRE(vars.size() == 1);
    CHECK(vars[0].key == "URL");
    CHECK(vars[0].value == "postgres://host:5432/db?opt=1");
}
