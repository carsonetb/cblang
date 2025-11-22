#pragma once 

#include "cblang.h"
#include "lexical.h"
#include <vector>

namespace cblang::compiler {
    auto init(bool verbose = false) -> void;
    auto enable_verbose_logs() -> void;
    auto compile(const std::vector<lexical::Keyword>& keywords) -> Program;
}