#include <collab/env/load.hpp>
#include <collab/env/parse.hpp>
#include "expand.hpp"
#include "merge.hpp"

#include <algorithm>
#include <array>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

namespace collab::env {

// ── env file names we look for in each directory ─────────────────────────

static constexpr std::array env_filenames = {
    ".env",
    ".env.yaml",
    ".env.yml",
    ".env.json",
};

// ── load_file ────────────────────────────────────────────────────────────

auto load_file(const fs::path& path) -> std::vector<EnvVar> {
    if (!fs::exists(path)) return {};

    std::ifstream f(path);
    if (!f) return {};

    std::ostringstream ss;
    ss << f.rdbuf();
    auto content = ss.str();

    auto ext = path.extension().string();
    if (ext == ".yaml" || ext == ".yml") return parse_yaml(content);
    if (ext == ".json") return parse_json(content);
    return parse_dotenv(content);
}

// ── find_env_files ───────────────────────────────────────────────────────

auto find_env_files(const fs::path& from) -> std::vector<fs::path> {
    // Walk from 'from' up to the filesystem root, collecting directories
    std::vector<fs::path> dirs;
    auto current = fs::canonical(from);
    while (true) {
        dirs.push_back(current);
        auto parent = current.parent_path();
        if (parent == current) break;  // reached root
        current = parent;
    }

    // Reverse so root is first (root → closest)
    std::reverse(dirs.begin(), dirs.end());

    // Scan each directory for env files
    std::vector<fs::path> found;
    for (auto& dir : dirs) {
        for (auto& name : env_filenames) {
            auto candidate = dir / name;
            if (fs::exists(candidate))
                found.push_back(candidate);
        }
    }

    return found;
}

// ── load ─────────────────────────────────────────────────────────────────

auto load(const fs::path& from) -> std::vector<EnvVar> {
    auto files = find_env_files(from);

    std::vector<EnvVar> all_vars;
    for (auto& file : files) {
        auto file_vars = load_file(file);
        all_vars.insert(all_vars.end(), file_vars.begin(), file_vars.end());
    }

    auto merged = detail::merge(std::move(all_vars));
    detail::expand(merged);
    return merged;
}

// ── merge (public) ───────────────────────────────────────────────────────

auto merge(std::vector<EnvVar> vars) -> std::vector<EnvVar> {
    return detail::merge(std::move(vars));
}

// ── expand (public) ──────────────────────────────────────────────────────

auto expand(std::vector<EnvVar>& vars) -> void {
    detail::expand(vars);
}

// ── apply ────────────────────────────────────────────────────────────────

auto apply(const std::vector<EnvVar>& vars) -> void {
    for (auto& v : vars) {
        if (v.value.empty()) {
#ifdef _WIN32
            _putenv_s(v.key.c_str(), "");
#else
            unsetenv(v.key.c_str());
#endif
        } else {
#ifdef _WIN32
            _putenv_s(v.key.c_str(), v.value.c_str());
#else
            setenv(v.key.c_str(), v.value.c_str(), 1);
#endif
        }
    }
}

}  // namespace collab::env
