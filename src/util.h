#pragma once

#include <algorithm>
#include <string>
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

// https://stackoverflow.com/questions/3418231/replace-part-of-a-string-with-another-string
auto replace_all(std::string& str, const std::string& from, const std::string& to) -> void {
    if (from.empty()) {
        return;
    }
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
}