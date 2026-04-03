#include <dotenv/parse.hpp>

#include <nlohmann/json.hpp>

namespace dotenv {

auto parse_json(std::string_view content) -> std::vector<EnvironmentVariable> {
    std::vector<EnvironmentVariable> vars;
    if (content.empty()) return vars;

    try {
        auto j = nlohmann::json::parse(content);
        if (!j.is_object()) return vars;

        for (auto& [key, val] : j.items()) {
            std::string value;
            if (val.is_null()) {
                // null → empty string (unset semantic)
            } else if (val.is_string()) {
                value = val.get<std::string>();
            } else {
                value = val.dump();
            }
            vars.push_back({key, std::move(value)});
        }
    } catch (...) {
        // Invalid JSON → return empty
    }

    return vars;
}

}  // namespace dotenv
