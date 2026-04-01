#pragma once

#include <collab/env/env_var.hpp>

#include <string_view>
#include <vector>

namespace collab::env {

/// Parse .env format content (KEY=VALUE, comments, quotes).
auto parse_dotenv(std::string_view content) -> std::vector<EnvVar>;

/// Parse YAML format content (top-level map of key: value).
auto parse_yaml(std::string_view content) -> std::vector<EnvVar>;

/// Parse JSON format content (top-level object of "key": value).
auto parse_json(std::string_view content) -> std::vector<EnvVar>;

}  // namespace collab::env
