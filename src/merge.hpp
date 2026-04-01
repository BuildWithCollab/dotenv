#pragma once

#include <collab/env/env_var.hpp>

#include <vector>

namespace collab::env::detail {

/// Merge env vars: last write wins, case-insensitive key matching.
/// Preserves insertion order of first occurrence.
auto merge(std::vector<EnvVar> vars) -> std::vector<EnvVar>;

}  // namespace collab::env::detail
