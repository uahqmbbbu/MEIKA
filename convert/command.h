#pragma once

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

inline std::string trim(const std::string &s) {
    size_t b = 0, e = s.size();
    while (b < e && std::isspace(s[b])) ++b;
    while (e > b && std::isspace(s[e - 1])) --e;
    return s.substr(b, e - b);
}

inline std::string lower(const std::string &s) {
    std::string r = s;
    for (auto &c : r) c = std::tolower(c);
    return r;
}

inline std::string readLine(const std::string &prompt) {
    if (!prompt.empty()) std::cout << prompt << std::flush;
    std::string line;
    if (!std::getline(std::cin, line)) std::exit(0);
    line = trim(line);
    if (line == ":q") {
        std::cout << "Bye.\n";
        std::exit(0);
    }
    return line;
}

inline std::vector<std::string> parseTokens(const std::string &line) {
    std::vector<std::string> tokens;
    std::stringstream ss(line);
    std::string tok;
    while (ss >> tok) tokens.push_back(tok);
    return tokens;
}

inline std::string expandVar(const std::string &line, const std::string &var, int value) {
    std::string pattern = "$" + var;
    std::string replacement = std::to_string(value);
    std::string result = line;
    size_t pos = 0;
    while ((pos = result.find(pattern, pos)) != std::string::npos) {
        result.replace(pos, pattern.size(), replacement);
        pos += replacement.size();
    }
    return result;
}

inline std::string detectFmt(const std::string &filename) {
    auto dot = filename.rfind('.');
    if (dot == std::string::npos) return "";
    auto ext = lower(filename.substr(dot));
    if (ext == ".pdb") return "pdb";
    if (ext == ".xyz") return "xyz";
    if (ext == ".engrad") return "engrad";
    if (ext == ".dcd") return "dcd";
    return "";
}

inline std::vector<int> parseIndices(const std::string &spec,
                                     size_t max_count) {
    std::vector<int> idx;
    std::stringstream ss(spec);
    std::string item;
    while (std::getline(ss, item, ',')) {
        if (item.empty()) continue;
        auto colon = item.find(':');
        if (colon != std::string::npos) {
            auto c2 = item.find(':', colon + 1);
            long long start = std::stoll(item.substr(0, colon));
            long long end, step = 1;
            if (c2 != std::string::npos) {
                end = std::stoll(item.substr(colon + 1, c2 - colon - 1));
                step = std::stoll(item.substr(c2 + 1));
            } else {
                end = std::stoll(item.substr(colon + 1));
            }
            if (start <= end)
                for (long long i = start; i <= end; i += step)
                    idx.push_back(i - 1);
            else
                for (long long i = start; i >= end; i -= step)
                    idx.push_back(i - 1);
        } else {
            idx.push_back(std::stoull(item) - 1);
        }
    }
    return idx;
}
