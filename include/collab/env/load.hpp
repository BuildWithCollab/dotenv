#pragma once

#include <collab/env/env_var.hpp>

#include <filesystem>
#include <vector>

namespace collab::env {

/// Load a single env file, auto-detecting format by extension.
/// .yaml/.yml → parse_yaml, .json → parse_json, anything else → parse_dotenv.
auto load_file(const std::filesystem::path& path) -> std::vector<EnvVar>;

/// Walk from 'from' to the filesystem root, collecting all env files found.
/// Returns paths ordered root → closest (ready for cascading merge).
auto find_env_files(const std::filesystem::path& from = std::filesystem::current_path())
    -> std::vector<std::filesystem::path>;

/// Walk from 'from' to root, load all env files, merge + expand.
/// Closest wins. This is the main entry point (like python-dotenv's dotenv_values).
auto load(const std::filesystem::path& from = std::filesystem::current_path())
    -> std::vector<EnvVar>;

/// Merge env vars: last write wins, case-insensitive key matching.
/// Preserves insertion order of first occurrence.
auto merge(std::vector<EnvVar> vars) -> std::vector<EnvVar>;

/// Expand ${VAR} references in values.
/// Resolves from the vars list first (case-insensitive), then falls back to
/// the real environment. Unknown refs are preserved as-is.
auto expand(std::vector<EnvVar>& vars) -> void;

/// Apply env vars to the current process. Empty value = unset.
auto apply(const std::vector<EnvVar>& vars) -> void;

}  // namespace collab::env
