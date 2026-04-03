#include <catch2/catch_test_macros.hpp>

#include <dotenv/load.hpp>

#include <cstdlib>
#include <string>

using namespace dotenv;

static auto get_env(const char* key) -> std::string {
    auto val = std::getenv(key);
    return val ? std::string(val) : "";
}

// ── basic apply ──────────────────────────────────────────────────────────

TEST_CASE("apply: sets environment variables", "[apply]") {
    apply({{"COLLAB_ENV_TEST_A", "hello"}, {"COLLAB_ENV_TEST_B", "world"}});
    CHECK(get_env("COLLAB_ENV_TEST_A") == "hello");
    CHECK(get_env("COLLAB_ENV_TEST_B") == "world");

    // Cleanup
    apply({{"COLLAB_ENV_TEST_A", ""}, {"COLLAB_ENV_TEST_B", ""}});
}

TEST_CASE("apply: overwrites existing environment variable", "[apply]") {
    apply({{"COLLAB_ENV_TEST_OVERWRITE", "first"}});
    CHECK(get_env("COLLAB_ENV_TEST_OVERWRITE") == "first");

    apply({{"COLLAB_ENV_TEST_OVERWRITE", "second"}});
    CHECK(get_env("COLLAB_ENV_TEST_OVERWRITE") == "second");

    // Cleanup
    apply({{"COLLAB_ENV_TEST_OVERWRITE", ""}});
}

// ── unset (empty value) ──────────────────────────────────────────────────

TEST_CASE("apply: empty value unsets variable", "[apply]") {
    // Set it first
    apply({{"COLLAB_ENV_TEST_UNSET", "exists"}});
    CHECK(get_env("COLLAB_ENV_TEST_UNSET") == "exists");

    // Unset it
    apply({{"COLLAB_ENV_TEST_UNSET", ""}});
    // On Windows _putenv_s with "" sets to empty, on Unix unsetenv removes it
    // Either way, getenv should return empty or null
    auto val = std::getenv("COLLAB_ENV_TEST_UNSET");
#ifdef _WIN32
    // Windows: _putenv_s("key", "") sets key to empty string
    CHECK((val == nullptr || std::string(val).empty()));
#else
    CHECK(val == nullptr);
#endif
}

// ── does not clobber unrelated vars ──────────────────────────────────────

TEST_CASE("apply: does not affect unrelated variables", "[apply]") {
    auto path_before = get_env("PATH");
    apply({{"COLLAB_ENV_TEST_ISOLATED", "yes"}});
    CHECK(get_env("PATH") == path_before);

    // Cleanup
    apply({{"COLLAB_ENV_TEST_ISOLATED", ""}});
}

// ── empty input ──────────────────────────────────────────────────────────

TEST_CASE("apply: empty vector does nothing", "[apply]") {
    auto path_before = get_env("PATH");
    apply({});
    CHECK(get_env("PATH") == path_before);
}
