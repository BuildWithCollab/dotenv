#pragma once

#include <collab/env/env_var.hpp>

#include <vector>

namespace collab::env::detail {

/// Expand ${VAR} references in values.
/// Resolves from the vars list first (case-insensitive), then falls back to
/// the real environment. Unknown refs are preserved as-is.
/// Iterates until stable (max 20 passes) to handle chained refs.
auto expand(std::vector<EnvVar>& vars) -> void;

}  // namespace collab::env::detail
