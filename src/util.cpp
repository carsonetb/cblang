#include "util.h"

auto replace_all(std::string& str, const std::string& from, const std::string& replace_to) -> void {
    if (from.empty()) {
        return;
    }
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), replace_to);
        start_pos += replace_to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
}

auto repeat_string(std::string str, const std::size_t n) -> std::string
{
    if (n == 0) {
        str.clear();
        str.shrink_to_fit();
        return str;
    } 
    if (n == 1 || str.empty()) {
        return str;
    }
    const auto period = str.size();
    if (period == 1) {
        str.append(n - 1, str.front());
        return str;
    }
    str.reserve(period * n);
    std::size_t mmm {2};
    for (; mmm < n; mmm *= 2) { str += str;}
    str.append(str.c_str(), (n - (mmm / 2)) * period);
    return str;
}