#include <catch2/catch_test_macros.hpp>

#include <collab/env/parse.hpp>

using namespace collab::env;

// ── empty / whitespace / comments ────────────────────────────────────────

TEST_CASE("parse_dotenv: empty string returns no vars", "[parse_dotenv]") {
    auto vars = parse_dotenv("");
    CHECK(vars.empty());
}

TEST_CASE("parse_dotenv: whitespace-only returns no vars", "[parse_dotenv]") {
    auto vars = parse_dotenv("   \n\t\n  \n");
    CHECK(vars.empty());
}

TEST_CASE("parse_dotenv: comment lines are skipped", "[parse_dotenv]") {
    auto vars = parse_dotenv("# this is a comment\n  # indented comment\n");
    CHECK(vars.empty());
}

// ── basic KEY=VALUE ──────────────────────────────────────────────────────

TEST_CASE("parse_dotenv: simple key=value", "[parse_dotenv]") {
    auto vars = parse_dotenv("FOO=bar\n");
    REQUIRE(vars.size() == 1);
    CHECK(vars[0].key == "FOO");
    CHECK(vars[0].value == "bar");
}

TEST_CASE("parse_dotenv: multiple key=value pairs", "[parse_dotenv]") {
    auto vars = parse_dotenv("A=1\nB=2\nC=3\n");
    REQUIRE(vars.size() == 3);
    CHECK(vars[0].key == "A");
    CHECK(vars[0].value == "1");
    CHECK(vars[1].key == "B");
    CHECK(vars[1].value == "2");
    CHECK(vars[2].key == "C");
    CHECK(vars[2].value == "3");
}

TEST_CASE("parse_dotenv: no trailing newline", "[parse_dotenv]") {
    auto vars = parse_dotenv("FOO=bar");
    REQUIRE(vars.size() == 1);
    CHECK(vars[0].key == "FOO");
    CHECK(vars[0].value == "bar");
}

// ── whitespace handling ──────────────────────────────────────────────────

TEST_CASE("parse_dotenv: spaces around = are trimmed", "[parse_dotenv]") {
    auto vars = parse_dotenv("FOO = bar\n");
    REQUIRE(vars.size() == 1);
    CHECK(vars[0].key == "FOO");
    CHECK(vars[0].value == "bar");
}

TEST_CASE("parse_dotenv: tabs around = are trimmed", "[parse_dotenv]") {
    auto vars = parse_dotenv("FOO\t=\tbar\n");
    REQUIRE(vars.size() == 1);
    CHECK(vars[0].key == "FOO");
    CHECK(vars[0].value == "bar");
}

TEST_CASE("parse_dotenv: leading whitespace on line is trimmed", "[parse_dotenv]") {
    auto vars = parse_dotenv("  FOO=bar\n");
    REQUIRE(vars.size() == 1);
    CHECK(vars[0].key == "FOO");
    CHECK(vars[0].value == "bar");
}

// ── quoted values ────────────────────────────────────────────────────────

TEST_CASE("parse_dotenv: double-quoted value", "[parse_dotenv]") {
    auto vars = parse_dotenv("FOO=\"hello world\"\n");
    REQUIRE(vars.size() == 1);
    CHECK(vars[0].key == "FOO");
    CHECK(vars[0].value == "hello world");
}

TEST_CASE("parse_dotenv: single-quoted value", "[parse_dotenv]") {
    auto vars = parse_dotenv("FOO='hello world'\n");
    REQUIRE(vars.size() == 1);
    CHECK(vars[0].key == "FOO");
    CHECK(vars[0].value == "hello world");
}

TEST_CASE("parse_dotenv: quoted value with spaces around =", "[parse_dotenv]") {
    auto vars = parse_dotenv("FOO = \"hello world\"\n");
    REQUIRE(vars.size() == 1);
    CHECK(vars[0].key == "FOO");
    CHECK(vars[0].value == "hello world");
}

TEST_CASE("parse_dotenv: empty double-quoted value", "[parse_dotenv]") {
    auto vars = parse_dotenv("FOO=\"\"\n");
    REQUIRE(vars.size() == 1);
    CHECK(vars[0].key == "FOO");
    CHECK(vars[0].value == "");
}

// ── empty values (unset semantic) ────────────────────────────────────────

TEST_CASE("parse_dotenv: empty value (KEY=)", "[parse_dotenv]") {
    auto vars = parse_dotenv("FOO=\n");
    REQUIRE(vars.size() == 1);
    CHECK(vars[0].key == "FOO");
    CHECK(vars[0].value == "");
}

TEST_CASE("parse_dotenv: empty value with spaces (KEY = )", "[parse_dotenv]") {
    auto vars = parse_dotenv("FOO = \n");
    REQUIRE(vars.size() == 1);
    CHECK(vars[0].key == "FOO");
    CHECK(vars[0].value == "");
}

// ── edge cases ───────────────────────────────────────────────────────────

TEST_CASE("parse_dotenv: value containing = sign", "[parse_dotenv]") {
    auto vars = parse_dotenv("URL=postgres://host:5432/db?opt=1\n");
    REQUIRE(vars.size() == 1);
    CHECK(vars[0].key == "URL");
    CHECK(vars[0].value == "postgres://host:5432/db?opt=1");
}

TEST_CASE("parse_dotenv: line without = is skipped", "[parse_dotenv]") {
    auto vars = parse_dotenv("NOT_A_VAR\nFOO=bar\n");
    REQUIRE(vars.size() == 1);
    CHECK(vars[0].key == "FOO");
    CHECK(vars[0].value == "bar");
}

TEST_CASE("parse_dotenv: blank lines between vars", "[parse_dotenv]") {
    auto vars = parse_dotenv("A=1\n\n\nB=2\n");
    REQUIRE(vars.size() == 2);
    CHECK(vars[0].key == "A");
    CHECK(vars[1].key == "B");
}

TEST_CASE("parse_dotenv: mixed content (comments, blanks, vars)", "[parse_dotenv]") {
    auto vars = parse_dotenv(
        "# Database config\n"
        "DB_HOST=localhost\n"
        "DB_PORT=5432\n"
        "\n"
        "# Secret\n"
        "SECRET=\"my secret\"\n"
        "EMPTY=\n"
    );
    REQUIRE(vars.size() == 4);
    CHECK(vars[0].key == "DB_HOST");
    CHECK(vars[0].value == "localhost");
    CHECK(vars[1].key == "DB_PORT");
    CHECK(vars[1].value == "5432");
    CHECK(vars[2].key == "SECRET");
    CHECK(vars[2].value == "my secret");
    CHECK(vars[3].key == "EMPTY");
    CHECK(vars[3].value == "");
}

TEST_CASE("parse_dotenv: export prefix is handled", "[parse_dotenv]") {
    auto vars = parse_dotenv("export FOO=bar\n");
    REQUIRE(vars.size() == 1);
    CHECK(vars[0].key == "FOO");
    CHECK(vars[0].value == "bar");
}

TEST_CASE("parse_dotenv: windows \\r\\n line endings", "[parse_dotenv]") {
    auto vars = parse_dotenv("FOO=bar\r\nBAZ=qux\r\n");
    REQUIRE(vars.size() == 2);
    CHECK(vars[0].key == "FOO");
    CHECK(vars[0].value == "bar");
    CHECK(vars[1].key == "BAZ");
    CHECK(vars[1].value == "qux");
}

TEST_CASE("parse_dotenv: mixed \\n and \\r\\n line endings", "[parse_dotenv]") {
    auto vars = parse_dotenv("A=1\r\nB=2\nC=3\r\n");
    REQUIRE(vars.size() == 3);
    CHECK(vars[0].value == "1");
    CHECK(vars[1].value == "2");
    CHECK(vars[2].value == "3");
}
