#include <catch2/catch_test_macros.hpp>

#include "merge.hpp"

using namespace collab::env;
using namespace collab::env::detail;

// ── basic behavior ───────────────────────────────────────────────────────

TEST_CASE("merge: empty input returns empty", "[merge]") {
    auto result = merge({});
    CHECK(result.empty());
}

TEST_CASE("merge: single var passes through", "[merge]") {
    auto result = merge({{"FOO", "bar"}});
    REQUIRE(result.size() == 1);
    CHECK(result[0].key == "FOO");
    CHECK(result[0].value == "bar");
}

TEST_CASE("merge: no duplicates preserves all vars in order", "[merge]") {
    auto result = merge({{"A", "1"}, {"B", "2"}, {"C", "3"}});
    REQUIRE(result.size() == 3);
    CHECK(result[0].key == "A");
    CHECK(result[1].key == "B");
    CHECK(result[2].key == "C");
}

// ── last write wins ──────────────────────────────────────────────────────

TEST_CASE("merge: duplicate keys — last write wins", "[merge]") {
    auto result = merge({{"FOO", "first"}, {"BAR", "only"}, {"FOO", "second"}});
    REQUIRE(result.size() == 2);
    // FOO appears at its first position but with the last value
    CHECK(result[0].key == "FOO");
    CHECK(result[0].value == "second");
    CHECK(result[1].key == "BAR");
    CHECK(result[1].value == "only");
}

TEST_CASE("merge: triple duplicate — last wins", "[merge]") {
    auto result = merge({{"X", "1"}, {"X", "2"}, {"X", "3"}});
    REQUIRE(result.size() == 1);
    CHECK(result[0].key == "X");
    CHECK(result[0].value == "3");
}

// ── case-insensitive ─────────────────────────────────────────────────────

TEST_CASE("merge: case-insensitive key matching", "[merge]") {
    auto result = merge({{"foo", "lower"}, {"FOO", "upper"}});
    REQUIRE(result.size() == 1);
    CHECK(result[0].value == "upper");
}

TEST_CASE("merge: mixed case keys — first occurrence key preserved, last value wins", "[merge]") {
    auto result = merge({{"Foo", "first"}, {"fOO", "second"}, {"FOO", "third"}});
    REQUIRE(result.size() == 1);
    // Key from first occurrence is preserved
    CHECK(result[0].key == "Foo");
    CHECK(result[0].value == "third");
}

// ── empty value (unset) ──────────────────────────────────────────────────

TEST_CASE("merge: empty value override (unset semantic)", "[merge]") {
    auto result = merge({{"FOO", "bar"}, {"FOO", ""}});
    REQUIRE(result.size() == 1);
    CHECK(result[0].key == "FOO");
    CHECK(result[0].value == "");
}
