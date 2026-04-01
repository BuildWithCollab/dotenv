#include <collab/env/parse.hpp>

#include <sstream>
#include <string>

namespace collab::env {

auto parse_dotenv(std::string_view content) -> std::vector<EnvVar> {
    std::vector<EnvVar> vars;
    std::istringstream stream{std::string(content)};
    std::string line;

    while (std::getline(stream, line)) {
        // Strip trailing \r (Windows line endings)
        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        // Find first non-whitespace
        auto start = line.find_first_not_of(" \t");
        if (start == std::string::npos) continue;
        if (line[start] == '#') continue;

        // Strip optional "export " prefix
        constexpr std::string_view export_prefix = "export ";
        auto working = std::string_view{line}.substr(start);
        if (working.starts_with(export_prefix))
            working.remove_prefix(export_prefix.size());

        // Skip leading whitespace again after export
        auto ws = working.find_first_not_of(" \t");
        if (ws == std::string_view::npos) continue;
        working.remove_prefix(ws);

        // Find the = separator
        auto eq = working.find('=');
        if (eq == std::string_view::npos) continue;

        // Extract key, trim trailing whitespace
        auto key = working.substr(0, eq);
        while (!key.empty() && (key.back() == ' ' || key.back() == '\t'))
            key.remove_suffix(1);
        if (key.empty()) continue;

        // Extract value, trim leading whitespace
        auto val_part = working.substr(eq + 1);
        auto val_start = val_part.find_first_not_of(" \t");
        std::string value;
        if (val_start != std::string_view::npos) {
            val_part = val_part.substr(val_start);
            value = std::string(val_part);

            // Strip matching quotes
            if (value.size() >= 2) {
                char q = value.front();
                if ((q == '"' || q == '\'') && value.back() == q)
                    value = value.substr(1, value.size() - 2);
            }
        }

        vars.push_back({std::string(key), std::move(value)});
    }

    return vars;
}

}  // namespace collab::env
