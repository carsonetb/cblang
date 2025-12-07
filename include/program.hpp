#pragma once

#include "objects.hpp"
#include <memory>
#include <string>
#include <unordered_map>

namespace cblang::program {
    struct Scope {
        std::unordered_map<std::string, std::shared_ptr<objects::Object>> defined_objects;
    };
}