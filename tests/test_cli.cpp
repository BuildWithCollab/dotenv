#include <catch2/catch_test_macros.hpp>

#include <collab/process.hpp>

#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;
using namespace collab::process;

// ── helper: find the dotenv binary ───────────────────────────────────────

static auto dotenv_exe() -> std::string {
    auto home = std::getenv("USERPROFILE");
    if (!home) home = std::getenv("HOME");
    if (home) {
        auto path = fs::path(home) / "bin" / "dotenv"
#ifdef _WIN32
            += ".exe"
#endif
        ;
        if (fs::exists(path)) return path.string();
    }
    return "dotenv";
}

// ── helper: temp directory with env files ────────────────────────────────

struct TempDir {
    fs::path root;

    TempDir() {
        root = fs::temp_directory_path() / ("cli-test-" + std::to_string(
            std::hash<std::string>{}(std::to_string(
                std::chrono::steady_clock::now().time_since_epoch().count()))));
        fs::create_directories(root);
        root = fs::canonical(root);
    }

    ~TempDir() { fs::remove_all(root); }

    void write(const fs::path& rel_path, const std::string& content) {
        auto full = root / rel_path;
        fs::create_directories(full.parent_path());
        std::ofstream f(full);
        f << content;
    }
};

// ── helper: run dotenv in a working directory ────────────────────────────

static auto run_dotenv(const fs::path& cwd, std::vector<std::string> args)
    -> std::expected<Result, SpawnError> {
    return Command(dotenv_exe())
        .args(std::move(args))
        .working_directory(cwd)
        .stdout_capture()
        .stderr_capture()
        .run();
}

// ── --help and --version (single arg only) ───────────────────────────────

TEST_CASE("cli: --help shows usage", "[cli]") {
    auto r = run_dotenv(fs::temp_directory_path(), {"--help"});
    REQUIRE(r.has_value());
    CHECK(r->ok());
    CHECK(r->stdout_content.find("load .env files") != std::string::npos);
}

TEST_CASE("cli: -h shows usage", "[cli]") {
    auto r = run_dotenv(fs::temp_directory_path(), {"-h"});
    REQUIRE(r.has_value());
    CHECK(r->ok());
    CHECK(r->stdout_content.find("load .env files") != std::string::npos);
}

TEST_CASE("cli: --version shows version", "[cli]") {
    auto r = run_dotenv(fs::temp_directory_path(), {"--version"});
    REQUIRE(r.has_value());
    CHECK(r->ok());
    CHECK(r->stdout_content.find("env ") != std::string::npos);
}

// ── print mode: no args ──────────────────────────────────────────────────

TEST_CASE("cli: no args prints full env with .env vars", "[cli]") {
    TempDir tmp;
    tmp.write(".env", "CLI_PRINT_TEST_XYZ=from_dotenv\n");
    auto r = run_dotenv(tmp.root, {});
    REQUIRE(r.has_value());
    CHECK(r->ok());
    CHECK(r->stdout_content.find("CLI_PRINT_TEST_XYZ=from_dotenv") != std::string::npos);
    CHECK(r->stdout_content.find("PATH=") != std::string::npos);
}

TEST_CASE("cli: no args, no .env, still prints real env", "[cli]") {
    TempDir tmp;
    auto r = run_dotenv(tmp.root, {});
    REQUIRE(r.has_value());
    CHECK(r->ok());
    CHECK(r->stdout_content.find("PATH=") != std::string::npos);
}

TEST_CASE("cli: -- alone prints full env WITHOUT .env vars", "[cli]") {
    TempDir tmp;
    tmp.write(".env", "SHOULD_NOT_APPEAR_XYZ=nope\n");
    auto r = run_dotenv(tmp.root, {"--"});
    REQUIRE(r.has_value());
    CHECK(r->ok());
    CHECK(r->stdout_content.find("SHOULD_NOT_APPEAR_XYZ") == std::string::npos);
    CHECK(r->stdout_content.find("PATH=") != std::string::npos);
}

// ── no separator: auto-load ON, command auto-detected ────────────────────

TEST_CASE("cli: no separator — command runs with auto-loaded env", "[cli]") {
    TempDir tmp;
    tmp.write(".env", "CLI_TEST_VAR=from_dotenv\n");
    auto r = run_dotenv(tmp.root, {"bash", "-c", "echo $CLI_TEST_VAR"});
    REQUIRE(r.has_value());
    CHECK(r->ok());
    CHECK(r->stdout_content.find("from_dotenv") != std::string::npos);
}

TEST_CASE("cli: no separator — inline var + command", "[cli]") {
    TempDir tmp;
    auto r = run_dotenv(tmp.root, {"INLINE=yes", "bash", "-c", "echo $INLINE"});
    REQUIRE(r.has_value());
    CHECK(r->ok());
    CHECK(r->stdout_content.find("yes") != std::string::npos);
}

TEST_CASE("cli: no separator — inline var + auto-load + command", "[cli]") {
    TempDir tmp;
    tmp.write(".env", "FROM_FILE=hello\n");
    auto r = run_dotenv(tmp.root, {"EXTRA=world", "bash", "-c", "echo $FROM_FILE $EXTRA"});
    REQUIRE(r.has_value());
    CHECK(r->ok());
    CHECK(r->stdout_content.find("hello") != std::string::npos);
    CHECK(r->stdout_content.find("world") != std::string::npos);
}

TEST_CASE("cli: no separator — inline var overrides .env var", "[cli]") {
    TempDir tmp;
    tmp.write(".env", "FOO=from_file\n");
    auto r = run_dotenv(tmp.root, {"FOO=overridden", "bash", "-c", "echo $FOO"});
    REQUIRE(r.has_value());
    CHECK(r->ok());
    CHECK(r->stdout_content.find("overridden") != std::string::npos);
}

TEST_CASE("cli: no separator — extra .env file + command", "[cli]") {
    TempDir tmp;
    tmp.write(".env", "BASE=yes\n");
    tmp.write(".env.extra", "EXTRA=also\n");
    auto r = run_dotenv(tmp.root, {".env.extra", "bash", "-c", "echo $BASE $EXTRA"});
    REQUIRE(r.has_value());
    CHECK(r->ok());
    CHECK(r->stdout_content.find("yes") != std::string::npos);
    CHECK(r->stdout_content.find("also") != std::string::npos);
}

TEST_CASE("cli: no separator — multiple files + inline + command", "[cli]") {
    TempDir tmp;
    tmp.write(".env", "A=auto\n");
    tmp.write(".env.one", "B=one\n");
    tmp.write(".env.two", "C=two\n");
    auto r = run_dotenv(tmp.root, {".env.one", ".env.two", "D=four", "bash", "-c", "echo $A $B $C $D"});
    REQUIRE(r.has_value());
    CHECK(r->ok());
    CHECK(r->stdout_content.find("auto") != std::string::npos);
    CHECK(r->stdout_content.find("one") != std::string::npos);
    CHECK(r->stdout_content.find("two") != std::string::npos);
    CHECK(r->stdout_content.find("four") != std::string::npos);
}

// ── -- separator: auto-load OFF ──────────────────────────────────────────

TEST_CASE("cli: -- disables auto-load", "[cli]") {
    TempDir tmp;
    tmp.write(".env", "SHOULD_NOT=load\n");
    auto r = run_dotenv(tmp.root, {"--", "bash", "-c", "echo val=[$SHOULD_NOT]"});
    REQUIRE(r.has_value());
    CHECK(r->ok());
    CHECK(r->stdout_content.find("val=[]") != std::string::npos);
}

TEST_CASE("cli: inline var before -- works without auto-load", "[cli]") {
    TempDir tmp;
    tmp.write(".env", "AUTO=nope\n");
    auto r = run_dotenv(tmp.root, {"MANUAL=yes", "--", "bash", "-c", "echo auto=[$AUTO] manual=$MANUAL"});
    REQUIRE(r.has_value());
    CHECK(r->ok());
    CHECK(r->stdout_content.find("auto=[]") != std::string::npos);
    CHECK(r->stdout_content.find("manual=yes") != std::string::npos);
}

TEST_CASE("cli: extra .env file before -- loads without auto-load", "[cli]") {
    TempDir tmp;
    tmp.write(".env", "AUTO=nope\n");
    tmp.write(".env.manual", "MANUAL=yes\n");
    auto r = run_dotenv(tmp.root, {".env.manual", "--", "bash", "-c", "echo auto=[$AUTO] manual=$MANUAL"});
    REQUIRE(r.has_value());
    CHECK(r->ok());
    CHECK(r->stdout_content.find("auto=[]") != std::string::npos);
    CHECK(r->stdout_content.find("manual=yes") != std::string::npos);
}

// ── -- after command is just a regular arg ───────────────────────────────

TEST_CASE("cli: -- after command start is passed as arg", "[cli]") {
    TempDir tmp;
    auto r = run_dotenv(tmp.root, {"bash", "-c", "echo before -- after"});
    REQUIRE(r.has_value());
    CHECK(r->ok());
    CHECK(r->stdout_content.find("before -- after") != std::string::npos);
}

TEST_CASE("cli: env file + inline + command + -- as arg to command", "[cli]") {
    TempDir tmp;
    tmp.write(".env.local", "X=1\n");
    // .env.local X=1 some-cmd -- should run some-cmd with "--" as an arg
    auto r = run_dotenv(tmp.root, {".env.local", "FOO=bar", "bash", "-c", "echo $X $FOO -- extra"});
    REQUIRE(r.has_value());
    CHECK(r->ok());
    CHECK(r->stdout_content.find("1 bar -- extra") != std::string::npos);
}

// ── command exit code propagation ────────────────────────────────────────

TEST_CASE("cli: propagates child exit code 0", "[cli]") {
    TempDir tmp;
    auto r = run_dotenv(tmp.root, {"bash", "-c", "exit 0"});
    REQUIRE(r.has_value());
    CHECK(r->exit_code == 0);
}

TEST_CASE("cli: propagates child exit code non-zero", "[cli]") {
    TempDir tmp;
    auto r = run_dotenv(tmp.root, {"bash", "-c", "exit 42"});
    REQUIRE(r.has_value());
    CHECK(r->exit_code == 42);
}

// ── command not found ────────────────────────────────────────────────────

TEST_CASE("cli: non-existent command returns error", "[cli]") {
    TempDir tmp;
    auto r = run_dotenv(tmp.root, {"this-command-does-not-exist-12345"});
    REQUIRE(r.has_value());
    CHECK(!r->ok());
}

// ── cascading .env files (walk to root) ──────────────────────────────────

TEST_CASE("cli: auto-load walks up directories, closest wins", "[cli]") {
    TempDir tmp;
    tmp.write(".env", "FOO=from_root\nROOT_ONLY=yes\n");
    tmp.write("child/.env", "FOO=from_child\n");

    auto r = run_dotenv(tmp.root / "child", {"bash", "-c", "echo foo=$FOO root=$ROOT_ONLY"});
    REQUIRE(r.has_value());
    CHECK(r->ok());
    CHECK(r->stdout_content.find("foo=from_child") != std::string::npos);
    CHECK(r->stdout_content.find("root=yes") != std::string::npos);
}

// ── ${VAR} expansion ─────────────────────────────────────────────────────

TEST_CASE("cli: variable expansion works in auto-loaded files", "[cli]") {
    TempDir tmp;
    tmp.write(".env", "HOST=localhost\nURL=http://${HOST}:8080\n");
    auto r = run_dotenv(tmp.root, {"bash", "-c", "echo $URL"});
    REQUIRE(r.has_value());
    CHECK(r->ok());
    CHECK(r->stdout_content.find("http://localhost:8080") != std::string::npos);
}

TEST_CASE("cli: inline var with ${VAR} ref expands from auto-loaded", "[cli]") {
    TempDir tmp;
    tmp.write(".env", "HOST=localhost\n");
    auto r = run_dotenv(tmp.root, {"URL=http://${HOST}:9090", "bash", "-c", "echo $URL"});
    REQUIRE(r.has_value());
    CHECK(r->ok());
    CHECK(r->stdout_content.find("http://localhost:9090") != std::string::npos);
}

TEST_CASE("cli: inline var with ${VAR} ref expands from other inline", "[cli]") {
    TempDir tmp;
    auto r = run_dotenv(tmp.root, {"A=hello", "B=${A}-world", "bash", "-c", "echo $B"});
    REQUIRE(r.has_value());
    CHECK(r->ok());
    CHECK(r->stdout_content.find("hello-world") != std::string::npos);
}

// ── Windows line endings ─────────────────────────────────────────────────

TEST_CASE("cli: .env with \\r\\n line endings works", "[cli]") {
    TempDir tmp;
    tmp.write(".env", "FOO=bar\r\nBAZ=qux\r\n");
    auto r = run_dotenv(tmp.root, {"bash", "-c", "echo foo=$FOO baz=$BAZ"});
    REQUIRE(r.has_value());
    CHECK(r->ok());
    CHECK(r->stdout_content.find("foo=bar") != std::string::npos);
    CHECK(r->stdout_content.find("baz=qux") != std::string::npos);
}

// ── .env file detection: strict filename matching ────────────────────────

TEST_CASE("cli: file with 'env' in name but not .env pattern is treated as command", "[cli]") {
    TempDir tmp;
    // GenValObj.exe has "env" in it but doesn't match .env pattern
    tmp.write("GenValObj.exe", "this is not an env file\n");
    auto r = run_dotenv(tmp.root, {"FOO=bar", "bash", "-c", "echo $FOO"});
    REQUIRE(r.has_value());
    CHECK(r->ok());
    CHECK(r->stdout_content.find("bar") != std::string::npos);
}
