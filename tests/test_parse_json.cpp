#include <catch2/catch_test_macros.hpp>

#include <collab/env/parse.hpp>

using namespace collab::env;

// ── empty / invalid ──────────────────────────────────────────────────────

TEST_CASE("parse_json: empty string returns no vars", "[parse_json]") {
    auto vars = parse_json("");
    CHECK(vars.empty());
}

TEST_CASE("parse_json: non-object root returns no vars", "[parse_json]") {
    auto vars = parse_json("[1, 2, 3]");
    CHECK(vars.empty());
}

TEST_CASE("parse_json: invalid json returns no vars", "[parse_json]") {
    auto vars = parse_json("{not json at all}");
    CHECK(vars.empty());
}

// ── basic values ─────────────────────────────────────────────────────────

TEST_CASE("parse_json: string values", "[parse_json]") {
    auto vars = parse_json(R"({"FOO": "bar", "BAZ": "qux"})");
    REQUIRE(vars.size() == 2);
    CHECK(vars[0].key == "BAZ");  // json objects are sorted by key
    CHECK(vars[0].value == "qux");
    CHECK(vars[1].key == "FOO");
    CHECK(vars[1].value == "bar");
}

TEST_CASE("parse_json: numeric values become strings", "[parse_json]") {
    auto vars = parse_json(R"({"PORT": 5432, "RATIO": 3.14})");
    REQUIRE(vars.size() == 2);
    CHECK(vars[0].key == "PORT");
    CHECK(vars[0].value == "5432");
    CHECK(vars[1].key == "RATIO");
    CHECK(vars[1].value == "3.14");
}

TEST_CASE("parse_json: boolean values become strings", "[parse_json]") {
    auto vars = parse_json(R"({"DEBUG": true, "VERBOSE": false})");
    REQUIRE(vars.size() == 2);
    CHECK(vars[0].key == "DEBUG");
    CHECK(vars[0].value == "true");
    CHECK(vars[1].key == "VERBOSE");
    CHECK(vars[1].value == "false");
}

TEST_CASE("parse_json: null value becomes empty string", "[parse_json]") {
    auto vars = parse_json(R"({"GONE": null})");
    REQUIRE(vars.size() == 1);
    CHECK(vars[0].key == "GONE");
    CHECK(vars[0].value == "");
}

// ── empty string value ───────────────────────────────────────────────────

TEST_CASE("parse_json: empty string value (unset semantic)", "[parse_json]") {
    auto vars = parse_json(R"({"EMPTY": ""})");
    REQUIRE(vars.size() == 1);
    CHECK(vars[0].key == "EMPTY");
    CHECK(vars[0].value == "");
}

// ── nested values ────────────────────────────────────────────────────────

TEST_CASE("parse_json: nested object is serialized", "[parse_json]") {
    auto vars = parse_json(R"({"CONFIG": {"host": "localhost", "port": 8080}})");
    REQUIRE(vars.size() == 1);
    CHECK(vars[0].key == "CONFIG");
    // The value should be JSON-serialized
    CHECK(vars[0].value.find("localhost") != std::string::npos);
    CHECK(vars[0].value.find("8080") != std::string::npos);
}
