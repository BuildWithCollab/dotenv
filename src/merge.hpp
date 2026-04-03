#pragma once

#include <dotenv/environment_variable.hpp>

#include <vector>

namespace dotenv::detail {

/// Merge env vars: last write wins, case-insensitive key matching.
/// Preserves insertion order of first occurrence.
auto merge(std::vector<EnvironmentVariable> vars) -> std::vector<EnvironmentVariable>;

}  // namespace dotenv::detail
