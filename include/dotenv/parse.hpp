#pragma once

#include <dotenv/environment_variable.hpp>

#include <string_view>
#include <vector>

namespace dotenv {

/// Parse .env format content (KEY=VALUE, comments, quotes).
auto parse_dotenv(std::string_view content) -> std::vector<EnvironmentVariable>;

/// Parse YAML format content (top-level map of key: value).
auto parse_yaml(std::string_view content) -> std::vector<EnvironmentVariable>;

/// Parse JSON format content (top-level object of "key": value).
auto parse_json(std::string_view content) -> std::vector<EnvironmentVariable>;

}  // namespace dotenv
