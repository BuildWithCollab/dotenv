#include "merge.hpp"

#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

namespace collab::env::detail {

auto merge(std::vector<EnvVar> vars) -> std::vector<EnvVar> {
    // Forward walk: track first position of each key (case-insensitive).
    // Always update value to last occurrence.
    struct Entry {
        size_t first_pos;
        std::string original_key;  // key name from first occurrence
        std::string value;         // value from last occurrence
    };

    std::unordered_map<std::string, Entry> map;
    size_t pos = 0;

    for (auto& v : vars) {
        auto lower_key = v.key;
        std::transform(lower_key.begin(), lower_key.end(), lower_key.begin(), ::tolower);

        auto it = map.find(lower_key);
        if (it == map.end()) {
            map[lower_key] = {pos, v.key, std::move(v.value)};
        } else {
            it->second.value = std::move(v.value);  // last write wins
        }
        pos++;
    }

    // Collect entries sorted by first_pos (preserves insertion order)
    std::vector<std::pair<size_t, EnvVar>> ordered;
    ordered.reserve(map.size());
    for (auto& [_, entry] : map)
        ordered.push_back({entry.first_pos, {std::move(entry.original_key), std::move(entry.value)}});

    std::sort(ordered.begin(), ordered.end(),
              [](auto& a, auto& b) { return a.first < b.first; });

    std::vector<EnvVar> result;
    result.reserve(ordered.size());
    for (auto& [_, ev] : ordered)
        result.push_back(std::move(ev));

    return result;
}

}  // namespace collab::env::detail
