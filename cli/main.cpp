// env — load .env files and optionally spawn a child process
//
// Usage:
//   env                                         Auto-load .env files, print vars
//   env -                                       Print vars (no auto-load)
//   env --version                               Show version
//   env -h | --help                             Show help
//   env -- cmd arg1 arg2                        Auto-load + run command
//   env - cmd arg1 arg2                         No auto-load + run command
//   env FOO=bar -- cmd arg1 arg2                Auto-load + inline var + run command
//   env FOO=bar - cmd arg1 arg2                 No auto-load + inline var + run command
//   env .env.extra -- cmd arg1 arg2             Auto-load + extra file + run command
//   env .env.extra - cmd arg1 arg2              No auto-load + file + run command
//   env .env.a .env.b FOO=x -- cmd arg1 arg2   Auto-load + files + inline + run command
//
// Separators (mutually exclusive):
//   --  = auto-load ON.  Before: config. After: command.
//   -   = auto-load OFF. Before: config. After: command.
//   No separator = no command, just print.

#ifdef _WIN32
    #define NOMINMAX
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#endif

#include <collab/env.hpp>
#include <collab/process.hpp>

#include <algorithm>
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

static auto looks_like_env_file(const std::string& arg) -> bool {
    if (!fs::exists(arg)) return false;
    auto lower = arg;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    return lower.find("env") != std::string::npos;
}

static void print_help() {
    std::cout <<
        "env — load .env files and run a command\n"
        "\n"
        "Usage:\n"
        "  env                                       Print all env vars (with .env files)\n"
        "  env -                                     Print all env vars (without .env files)\n"
        "  env -- cmd [args...]                      Run command (with .env files)\n"
        "  env - cmd [args...]                       Run command (without .env files)\n"
        "  env KEY=val .env.local -- cmd [args...]   Extra vars/files, then run command\n"
        "  env KEY=val .env.local - cmd [args...]    Same, but skip .env file loading\n"
        "  env -h | --help                           Show this help\n"
        "  env --version                             Show version\n"
        "\n"
        "Separators:\n"
        "  --  Load .env files from every parent directory, then run the command\n"
        "  -   Don't load any .env files automatically, then run the command\n"
        "\n"
        "  Everything before the separator is extra env files or KEY=value pairs.\n"
        "  Everything after the separator is the command to run.\n"
        "  Without a separator, nothing is run — vars are printed instead.\n"
        "\n"
        "How .env files are found:\n"
        "  Starting from your current directory, env looks in every parent\n"
        "  directory all the way up to the filesystem root. If a .env, .env.yaml,\n"
        "  .env.yml, or .env.json file exists in any of those directories, it gets\n"
        "  loaded. When the same variable appears in multiple files, the closest\n"
        "  file to your current directory wins.\n"
        "\n"
        "File formats:\n"
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

    // ── Find separator (- or --) ─────────────────────────────────────────

    int sep_pos = -1;
    bool auto_load = true;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--") {
            sep_pos = i;
            auto_load = true;
            break;
        }
        if (arg == "-") {
            sep_pos = i;
            auto_load = false;
            break;
        }
    }

    // ── Collect config (before separator) and command (after separator) ──

    std::vector<std::string> extra_files;
    std::vector<EnvVar> inline_vars;
    std::vector<std::string> cmd_args;

    // Before separator: env files and inline vars
    int config_end = (sep_pos > 0) ? sep_pos : argc;
    for (int i = 1; i < config_end; i++) {
        std::string arg = argv[i];
        if (looks_like_env_file(arg))
            extra_files.push_back(arg);
        else if (is_inline_var(arg)) {
            auto eq = arg.find('=');
            inline_vars.push_back({arg.substr(0, eq), arg.substr(eq + 1)});
        }
    }

    // After separator: the command
    if (sep_pos > 0) {
        for (int i = sep_pos + 1; i < argc; i++)
            cmd_args.push_back(argv[i]);
    }

    // ── Load env vars ────────────────────────────────────────────────────

    // Collect ALL raw vars first, then merge+expand once.
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

    // Single merge + expand across everything
    vars = collab::env::merge(std::move(vars));
    collab::env::expand(vars);

    collab::env::apply(vars);

    // ── Print mode: no separator, or separator with no command ─────────────

    if (sep_pos < 0 || cmd_args.empty()) {
        // Print the full environment (real env + .env overrides already applied)
#ifdef _WIN32
        auto env_block = GetEnvironmentStringsA();
        if (env_block) {
            for (auto p = env_block; *p; p += strlen(p) + 1) {
                // Skip entries starting with '=' (Windows internal vars)
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
