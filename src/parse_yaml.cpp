#include <dotenv/parse.hpp>

#include <yaml-cpp/yaml.h>

#include <sstream>

namespace dotenv {

auto parse_yaml(std::string_view content) -> std::vector<EnvironmentVariable> {
    std::vector<EnvironmentVariable> vars;
    if (content.empty()) return vars;

    try {
        auto root = YAML::Load(std::string(content));
        if (!root.IsMap()) return vars;

        for (auto it = root.begin(); it != root.end(); ++it) {
            auto key = it->first.as<std::string>();
            std::string value;

            if (it->second.IsNull()) {
                // null → empty string (unset semantic)
            } else if (it->second.IsScalar()) {
                value = it->second.as<std::string>();
                // yaml-cpp parses "null" as a Null node, but just in case
                if (value == "null" || value == "~") value = "";
            } else {
                // Complex values (maps, sequences) → serialize
                std::ostringstream ss;
                ss << it->second;
                value = ss.str();
            }

            vars.push_back({std::move(key), std::move(value)});
        }
    } catch (...) {
        // Invalid YAML → return empty
    }

    return vars;
}

}  // namespace dotenv
