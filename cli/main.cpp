// env — load .env files and run a command
//
// Usage:
//   env                                         Auto-load .env files, print full env
//   env --                                      Print full env (skip .env loading)
//   env -h | --help                             Show help
//   env --version                               Show version
//   env FOO=bar some-command --flag              Auto-load + inline + run command
//   env .env.local FOO=bar some-cmd              Auto-load + extra file + inline + run
//   env .env.local FOO=bar -- some-cmd           NO auto-load + extra file + inline + run
//
// No separator needed. First arg that's not a .env file and not KEY=val is the command.
// -- disables auto-load. Everything after -- is the command.

#ifdef _WIN32
    #define NOMINMAX
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#endif

#include <collab/env.hpp>
#include <collab/process.hpp>

#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace collab::env;

// ── helpers ──────────────────────────────────────────────────────────────

static auto is_inline_var(const std::string& arg) -> bool {
    auto eq = arg.find('=');
    return eq != std::string::npos && eq > 0;
}

static auto is_env_file(const std::string& arg) -> bool {
    // Strict: filename is exactly ".env" or starts with ".env."
    auto name = fs::path(arg).filename().string();
    return name == ".env" || name.starts_with(".env.");
}

static void print_help() {
    std::cout <<
        "env — load .env files and run a command\n"
        "\n"
        "Usage:\n"
        "  env                                       Print all env vars (with .env files)\n"
        "  env --                                    Print all env vars (without .env files)\n"
        "  env FOO=bar some-cmd [args...]            Run command with env vars\n"
        "  env .env.local FOO=bar some-cmd [args...] Extra file + vars, run command\n"
        "  env .env.local FOO=bar -- some-cmd        Same, but skip .env auto-loading\n"
        "  env -h | --help                           Show this help\n"
        "  env --version                             Show version\n"
        "\n"
        "How it works:\n"
        "  Arguments are read left to right. Env files (.env*) and KEY=value pairs\n"
        "  are collected. The first argument that is neither is the command — everything\n"
        "  from that point on is passed to it.\n"
        "\n"
        "  By default, .env files are automatically loaded from every parent directory\n"
        "  up to the filesystem root. The closest file wins on conflicts.\n"
        "\n"
        "  Use -- to disable auto-loading. Only files you list explicitly will be used.\n"
        "  If -- appears after the command has started, it is passed to the command.\n"
        "\n"
        "Env files:\n"
        "  Files named .env or starting with .env. (e.g. .env.local, .env.production)\n"
        "  are recognized as env files. Supported formats:\n"
        "\n"
        "  .env                         .env.yaml / .env.yml\n"
        "  ┌────────────────────────┐   ┌────────────────────────┐\n"
        "  │ DB_HOST=localhost      │   │ DB_HOST: localhost     │\n"
        "  │ DB_PORT=5432           │   │ DB_PORT: 5432          │\n"
        "  │ SECRET=\"my secret\"     │   │ SECRET: my secret      │\n"
        "  │ # comment              │   │                        │\n"
        "  │ UNSET_ME=              │   │ UNSET_ME:              │\n"
        "  └────────────────────────┘   └────────────────────────┘\n"
        "\n"
        "  .env.json                      Variable references\n"
        "  ┌────────────────────────────┐ ┌────────────────────────────────┐\n"
        "  │ {                          │ │ HOST=localhost                 │\n"
        "  │   \"DB_HOST\": \"localhost\",  │ │ PORT=5432                      │\n"
        "  │   \"DB_PORT\": 5432,         │ │ URL=http://${HOST}:${PORT}     │\n"
        "  │   \"SECRET\": \"my secret\",   │ └────────────────────────────────┘\n"
        "  │   \"UNSET_ME\": \"\"           │\n"
        "  │ }                          │\n"
        "  └────────────────────────────┘\n"
        "\n"
        "  KEY=value     Set a variable\n"
        "  KEY=          Unset a variable (empty value removes it)\n"
        "  ${VAR}        Reference another variable (from .env files or real env)\n"
        "  # comment     Lines starting with # are ignored (.env format only)\n"
        "  export KEY=   Optional 'export' prefix is accepted (.env format only)\n";
}

static void print_environment() {
#ifdef _WIN32
    auto env_block = GetEnvironmentStringsA();
    if (env_block) {
        for (auto p = env_block; *p; p += strlen(p) + 1) {
            if (*p != '=')
                std::cout << p << "\n";
        }
        FreeEnvironmentStringsA(env_block);
    }
#else
    extern char** environ;
    for (char** e = environ; *e; ++e)
        std::cout << *e << "\n";
#endif
}

// ── main ─────────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    // Single-arg flags
    if (argc == 2) {
        std::string arg = argv[1];
        if (arg == "-h" || arg == "--help") {
            print_help();
            return 0;
        }
        if (arg == "--version") {
            std::cout << "env " << collab::env::version_string << "\n";
            return 0;
        }
    }

    // ── Parse args ───────────────────────────────────────────────────────
    //
    // Walk left to right:
    //   --       → disable auto-load, everything after is the command
    //   .env*    → extra env file
    //   KEY=val  → inline var
    //   anything else → command starts here (everything from here on)

    bool auto_load = true;
    std::vector<std::string> extra_files;
    std::vector<EnvVar> inline_vars;
    std::vector<std::string> cmd_args;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "--") {
            auto_load = false;
            // Everything after -- is the command
            for (int j = i + 1; j < argc; j++)
                cmd_args.push_back(argv[j]);
            break;
        }

        if (is_env_file(arg)) {
            extra_files.push_back(arg);
            continue;
        }

        if (is_inline_var(arg)) {
            auto eq = arg.find('=');
            inline_vars.push_back({arg.substr(0, eq), arg.substr(eq + 1)});
            continue;
        }

        // First arg that's none of the above → command starts
        for (int j = i; j < argc; j++)
            cmd_args.push_back(argv[j]);
        break;
    }

    // ── Load env vars ────────────────────────────────────────────────────

    std::vector<EnvVar> vars;

    if (auto_load) {
        auto files = find_env_files();
        for (auto& f : files) {
            auto file_vars = load_file(f);
            vars.insert(vars.end(), file_vars.begin(), file_vars.end());
        }
    }

    for (auto& f : extra_files) {
        auto file_vars = load_file(f);
        vars.insert(vars.end(), file_vars.begin(), file_vars.end());
    }

    vars.insert(vars.end(), inline_vars.begin(), inline_vars.end());

    vars = collab::env::merge(std::move(vars));
    collab::env::expand(vars);
    collab::env::apply(vars);

    // ── No command → print mode ──────────────────────────────────────────

    if (cmd_args.empty()) {
        print_environment();
        return 0;
    }

    // ── Spawn command ────────────────────────────────────────────────────

    auto result = collab::process::Command(cmd_args[0])
        .args(std::vector<std::string>(cmd_args.begin() + 1, cmd_args.end()))
        .run();

    if (!result) {
        std::cerr << "env: failed to run '" << cmd_args[0] << "': "
                  << result.error().what() << "\n";
        return 1;
    }

    return result->exit_code.value_or(1);
}
