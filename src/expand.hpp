#pragma once

#include <dotenv/environment_variable.hpp>

#include <vector>

namespace dotenv::detail {

/// Expand ${VAR} references in values.
/// Resolves from the vars list first (case-insensitive), then falls back to
/// the real environment. Unknown refs are preserved as-is.
/// Iterates until stable (max 20 passes) to handle chained refs.
auto expand(std::vector<EnvironmentVariable>& vars) -> void;

}  // namespace dotenv::detail
