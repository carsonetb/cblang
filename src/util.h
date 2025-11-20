#pragma once

#include <algorithm>
#include <vector>

#define CHECK_INITIALIZED if (!initialized) { logger->error("cblang not initialized (call cblang::init)"); return; }

template <typename T>
auto vector_contains(std::vector<T> vec, T check) {
    return std::ranges::any_of(vec, [check](T val) -> auto { return val == check; });
}

template <typename T>
auto remove_from_vector(std::vector<T>& vec, const T& to_remove) {
    vec.erase(std::remove(vec.begin(), vec.end(), to_remove), vec.end());
}