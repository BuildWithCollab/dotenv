#include <catch2/catch_test_macros.hpp>

#include <dotenv/load.hpp>
#include <dotenv/parse.hpp>

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using namespace dotenv;

// ── helper: create a temp directory tree with env files ──────────────────

struct TempDir {
    fs::path root;

    TempDir() {
        root = fs::temp_directory_path() / ("collab-env-test-" + std::to_string(
            std::hash<std::string>{}(std::to_string(
                std::chrono::steady_clock::now().time_since_epoch().count()))));
        fs::create_directories(root);
        root = fs::canonical(root);  // resolve symlinks (/var→/private/var, RUNNER~1, etc.)
    }

    ~TempDir() { fs::remove_all(root); }

    void write(const fs::path& rel_path, const std::string& content) {
        auto full = root / rel_path;
        fs::create_directories(full.parent_path());
        std::ofstream f(full);
        f << content;
    }

    auto path(const fs::path& rel = {}) const -> fs::path {
        return rel.empty() ? root : root / rel;
    }
};

// ── load_file ────────────────────────────────────────────────────────────

TEST_CASE("load_file: loads .env file", "[load]") {
    TempDir tmp;
    tmp.write(".env", "FOO=bar\nBAZ=qux\n");
    auto vars = load_file(tmp.path(".env"));
    REQUIRE(vars.size() == 2);
    CHECK(vars[0].key == "FOO");
    CHECK(vars[1].key == "BAZ");
}

TEST_CASE("load_file: loads .env.yaml file", "[load]") {
    TempDir tmp;
    tmp.write("config.yaml", "PORT: 8080\n");
    auto vars = load_file(tmp.path("config.yaml"));
    REQUIRE(vars.size() == 1);
    CHECK(vars[0].key == "PORT");
    CHECK(vars[0].value == "8080");
}

TEST_CASE("load_file: loads .env.yml file", "[load]") {
    TempDir tmp;
    tmp.write("config.yml", "PORT: 9090\n");
    auto vars = load_file(tmp.path("config.yml"));
    REQUIRE(vars.size() == 1);
    CHECK(vars[0].key == "PORT");
}

TEST_CASE("load_file: loads .env.json file", "[load]") {
    TempDir tmp;
    tmp.write("config.json", R"({"API_KEY": "abc123"})");
    auto vars = load_file(tmp.path("config.json"));
    REQUIRE(vars.size() == 1);
    CHECK(vars[0].key == "API_KEY");
    CHECK(vars[0].value == "abc123");
}

TEST_CASE("load_file: non-existent file returns empty", "[load]") {
    auto vars = load_file("/tmp/does-not-exist-collab-env-test.env");
    CHECK(vars.empty());
}

// ── find_env_files ───────────────────────────────────────────────────────

TEST_CASE("find_env_files: finds .env in single directory", "[load]") {
    TempDir tmp;
    tmp.write(".env", "X=1\n");
    auto files = find_env_files(tmp.root);
    REQUIRE(files.size() == 1);
    CHECK(files[0].filename() == ".env");
}

TEST_CASE("find_env_files: finds all env file types", "[load]") {
    TempDir tmp;
    tmp.write(".env", "A=1\n");
    tmp.write(".env.yaml", "B: 2\n");
    tmp.write(".env.json", R"({"C": "3"})");
    auto files = find_env_files(tmp.root);
    CHECK(files.size() == 3);
}

TEST_CASE("find_env_files: walks up to parent directories", "[load]") {
    TempDir tmp;
    tmp.write(".env", "ROOT=1\n");
    tmp.write("child/.env", "CHILD=2\n");
    tmp.write("child/grandchild/.env.yaml", "GRAND: 3\n");

    auto files = find_env_files(tmp.path("child/grandchild"));
    // Should find: root/.env, child/.env, child/grandchild/.env.yaml
    // Ordered root → closest
    REQUIRE(files.size() == 3);
    CHECK(files[0].parent_path() == tmp.root);
    CHECK(files[1].parent_path() == tmp.path("child"));
    CHECK(files[2].parent_path() == tmp.path("child/grandchild"));
}

TEST_CASE("find_env_files: returns root-first ordering", "[load]") {
    TempDir tmp;
    tmp.write(".env", "ROOT=1\n");
    tmp.write("a/.env", "A=1\n");
    tmp.write("a/b/.env", "B=1\n");

    auto files = find_env_files(tmp.path("a/b"));
    REQUIRE(files.size() == 3);
    // Root's .env first, then a/.env, then a/b/.env
    CHECK(files[0] == tmp.path(".env"));
    CHECK(files[1] == tmp.path("a/.env"));
    CHECK(files[2] == tmp.path("a/b/.env"));
}

TEST_CASE("find_env_files: directory with no env files returns empty", "[load]") {
    TempDir tmp;
    tmp.write("readme.txt", "nothing here\n");
    auto files = find_env_files(tmp.root);
    CHECK(files.empty());
}

// ── load (full pipeline) ─────────────────────────────────────────────────

TEST_CASE("load: single .env file", "[load]") {
    TempDir tmp;
    tmp.write(".env", "FOO=bar\n");
    auto vars = load(tmp.root);
    REQUIRE(vars.size() == 1);
    CHECK(vars[0].key == "FOO");
    CHECK(vars[0].value == "bar");
}

TEST_CASE("load: cascading — closest wins", "[load]") {
    TempDir tmp;
    tmp.write(".env", "FOO=from-root\nROOT_ONLY=yes\n");
    tmp.write("child/.env", "FOO=from-child\nCHILD_ONLY=yes\n");

    auto vars = load(tmp.path("child"));
    // FOO should be from child (closest wins)
    bool found_foo = false;
    bool found_root_only = false;
    bool found_child_only = false;
    for (auto& v : vars) {
        if (v.key == "FOO") {
            CHECK(v.value == "from-child");
            found_foo = true;
        }
        if (v.key == "ROOT_ONLY") {
            CHECK(v.value == "yes");
            found_root_only = true;
        }
        if (v.key == "CHILD_ONLY") {
            CHECK(v.value == "yes");
            found_child_only = true;
        }
    }
    CHECK(found_foo);
    CHECK(found_root_only);
    CHECK(found_child_only);
}

TEST_CASE("load: cascading unset — child can clear parent var", "[load]") {
    TempDir tmp;
    tmp.write(".env", "SECRET=top-secret\n");
    tmp.write("child/.env", "SECRET=\n");

    auto vars = load(tmp.path("child"));
    REQUIRE(vars.size() == 1);
    CHECK(vars[0].key == "SECRET");
    CHECK(vars[0].value == "");  // unset by child
}

TEST_CASE("load: expansion works across cascaded files", "[load]") {
    TempDir tmp;
    tmp.write(".env", "HOST=localhost\n");
    tmp.write("child/.env", "URL=http://${HOST}:8080\n");

    auto vars = load(tmp.path("child"));
    bool found = false;
    for (auto& v : vars) {
        if (v.key == "URL") {
            CHECK(v.value == "http://localhost:8080");
            found = true;
        }
    }
    CHECK(found);
}

TEST_CASE("load: mixed formats in cascade", "[load]") {
    TempDir tmp;
    tmp.write(".env", "A=from-dotenv\n");
    tmp.write("child/.env.yaml", "B: from-yaml\n");
    tmp.write("child/grandchild/.env.json", R"({"C": "from-json"})");

    auto vars = load(tmp.path("child/grandchild"));
    REQUIRE(vars.size() == 3);
}

TEST_CASE("load: empty directory returns empty", "[load]") {
    TempDir tmp;
    auto vars = load(tmp.root);
    CHECK(vars.empty());
}
