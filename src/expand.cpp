#include "expand.hpp"

#include <algorithm>
#include <cstdlib>
#include <string>

namespace collab::env::detail {

static auto expand_value(const std::string& value,
                         const std::vector<EnvVar>& vars) -> std::string {
    std::string result;
    size_t i = 0;

    while (i < value.size()) {
        if (i + 1 < value.size() && value[i] == '$' && value[i + 1] == '{') {
            auto close = value.find('}', i + 2);
            if (close != std::string::npos) {
                auto var_name = value.substr(i + 2, close - i - 2);

                // Look up in our vars first (case-insensitive)
                auto lower_name = var_name;
                std::transform(lower_name.begin(), lower_name.end(),
                               lower_name.begin(), ::tolower);

                std::string found_value;
                bool found = false;
                for (auto& v : vars) {
                    auto lower_key = v.key;
                    std::transform(lower_key.begin(), lower_key.end(),
                                   lower_key.begin(), ::tolower);
                    if (lower_key == lower_name) {
                        found_value = v.value;
                        found = true;
                    }
                }

                // Fall back to real environment
                if (!found) {
                    auto env_val = std::getenv(var_name.c_str());
                    if (env_val) {
                        found_value = env_val;
                        found = true;
                    }
                }

                result += found ? found_value : value.substr(i, close - i + 1);
                i = close + 1;
                continue;
            }
        }
        result += value[i];
        i++;
    }

    return result;
}

auto expand(std::vector<EnvVar>& vars) -> void {
    constexpr int max_iterations = 20;

    for (int iter = 0; iter < max_iterations; iter++) {
        bool changed = false;
        for (auto& v : vars) {
            auto expanded = expand_value(v.value, vars);
            if (expanded != v.value) {
                v.value = std::move(expanded);
                changed = true;
            }
        }
        if (!changed) break;
    }
}

}  // namespace collab::env::detail
