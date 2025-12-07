#pragma once

#include <algorithm>
#include <string>
#include <vector>

#define CHECK_INITIALIZED if (!initialized) { logger->error("cblang not initialized (call cblang::init)"); return; }
#define TEMPLATED_EMPTY(name) std::make_shared<cblang::parser::Templated>(cblang::scanner::Token(cblang::scanner::IDENTIFIER, name, -1), std::vector<std::shared_ptr<cblang::parser::Templated>>())

template <typename T>
auto vector_contains(std::vector<T> vec, T check) {
    return std::ranges::any_of(vec, [check](T val) -> auto { return val == check; });
}

template <typename T>
auto remove_from_vector(std::vector<T>& vec, const T& to_remove) {
    vec.erase(std::remove(vec.begin(), vec.end(), to_remove), vec.end());
}

// https://stackoverflow.com/questions/3418231/replace-part-of-a-string-with-another-string
auto replace_all(std::string& str, const std::string& from, const std::string& replace_to) -> void;

// https://stackoverflow.com/questions/166630/how-can-i-repeat-a-string-a-variable-number-of-times-in-c (Answer by 'Daniel')
auto repeat_string(std::string str, std::size_t n) -> std::string;

// https://stackoverflow.com/questions/14265581/parse-split-a-string-in-c-using-string-delimiter-standard-c (Answer by 'Vincenzo Pii')
auto split(const std::string& split, const std::string& delimiter) -> std::vector<std::string>;