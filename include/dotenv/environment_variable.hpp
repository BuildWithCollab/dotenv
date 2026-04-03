#pragma once

#include <string>
#include <vector>

namespace dotenv {

struct EnvironmentVariable {
    std::string key;
    std::string value;
};

}  // namespace dotenv
