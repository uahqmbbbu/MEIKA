#pragma once
#ifndef MEIKA_XYZ
#define MEIKA_XYZ

#include "base.h"
#include "span.h"
#include "system.h"

#include <string>
#include <vector>

#include <fstream>

namespace meika {
namespace xyz {
auto readXYZ(span<const std::string> xyz) -> system::System;
auto readXYZ(const std::string &filename) -> system::System;
auto readMultiFrameXYZ(const std::string &filename)
    -> std::vector<system::System>;

auto outputXYZ(std::ostream &buffer, const std::vector<std::string> &title,
               const std::vector<real> &x, const std::vector<real> &y,
               const std::vector<real> &z, const int width, const int precision)
    -> void;

inline auto readXYZ(span<const std::string> xyz) -> system::System {
    std::size_t atom_number = std::stoul(xyz[0]);
    system::System sys(atom_number, system::System::InitMode::Reserve);

    std::string line;
    for (size_t i = 0; i < atom_number; ++i) {
        line = xyz[i + 2];
        const char *p = line.c_str();
        char *endptr;

        while (*p and std::isspace(static_cast<unsigned char>(*p)))
            p++; // 跳过开头空格
        const char *name_start = p;
        while (*p and !std::isspace(static_cast<unsigned char>(*p)))
            p++; // 寻找单词结尾
        size_t name_len = p - name_start;

        real x = meika::toReal(p, &endptr);
        if (p == endptr) continue;
        p = endptr;
        real y = meika::toReal(p, &endptr);
        if (p == endptr) continue;
        p = endptr;
        real z = meika::toReal(p, &endptr);
        if (p == endptr) continue;
        p = endptr;

        sys.name.emplace_back(name_start, name_len);
        sys.element.emplace_back(name_start, name_len);
        sys.x.emplace_back(x);
        sys.y.emplace_back(y);
        sys.z.emplace_back(z);
    }
    return sys;
}

inline auto readXYZ(const std::string &filename) -> system::System {
    std::ifstream xyz(filename);
    if (!xyz.is_open()) {
        std::cerr << "Error: Unable to open file: " << filename << std::endl;
    }
    std::vector<std::string> data;
    std::string line;
    while (std::getline(xyz, line)) { data.push_back(std::move(line)); }
    return readXYZ(span<const std::string>(data));
}

inline auto readMultiFrameXYZ(const std::string &filename)
    -> std::vector<system::System> {
    std::ifstream xyz(filename);
    if (!xyz.is_open()) {
        std::cerr << "Error: Unable to open file: " << filename << std::endl;
    }
    std::vector<std::string> frame_data;
    std::string line;
    while (std::getline(xyz, line)) { frame_data.push_back(std::move(line)); }
    const int atom_number = std::stoi(frame_data[0]);
    const std::size_t frame_number = frame_data.size() / (atom_number + 2);

    span<const std::string> frame_span(frame_data);
    std::vector<system::System> systems(frame_number);
#pragma omp parallel for
    for (size_t i = 0; i < frame_number; ++i) {
        size_t pos = i * (atom_number + 2);
        systems[i] = readXYZ(frame_span.subspan(pos, atom_number));
    }
    return systems;
}

inline auto outputXYZ(std::ostream &buffer,
                      const std::vector<std::string> &title,
                      const std::vector<real> &x, const std::vector<real> &y,
                      const std::vector<real> &z, const int width,
                      const int precision) -> void {
    std::size_t max_name = 0;
    for (const auto &t : title) {
        if (t.size() > max_name) max_name = t.size();
    }

    buffer << title.size() << "\n";
    buffer << "Extracted by meika\n";
    for (size_t i = 0; i < title.size(); ++i) {
        buffer << std::fixed << std::setprecision(precision);
        buffer << std::left << std::setw(max_name) << title[i] << " ";
        buffer << std::right << std::setw(width) << x[i] << " ";
        buffer << std::right << std::setw(width) << y[i] << " ";
        buffer << std::right << std::setw(width) << z[i] << "\n";
    }
}
} // namespace xyz
} // namespace meika
#endif
