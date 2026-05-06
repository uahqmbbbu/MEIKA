#pragma once
#ifndef MEIKA_AMBER
#define MEIKA_AMBER

#include <cctype>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "unit.h"

namespace meika {
namespace amber {

// Amber 电荷单位 → 原子单位转换因子
inline constexpr double Factor_au2amber = 18.2223;

struct FFParm {
    size_t n_atom;
    size_t n_type;
    std::vector<std::string> atom_name;
    std::vector<double> charges;
    std::vector<int> atom_type_index;
    std::vector<int> nonbonded_parm_index;
    std::vector<double> lj_acoef;
    std::vector<double> lj_bcoef;
};

template <typename T>
inline auto readParm(std::ifstream &parm, const size_t number,
                     const std::string feature) ->
    typename std::enable_if<std::is_arithmetic<T>::value, std::vector<T>>::type {
    std::vector<T> data;
    std::string line;
    while (std::getline(parm, line)) {
        if (line.find(feature) != std::string::npos) {
            std::getline(parm, line);
            break;
        }
    }
    while (data.size() < number and std::getline(parm, line)) {
        std::istringstream iss(line);
        T _value;
        while (iss >> _value and data.size() < number) {
            data.push_back(std::move(_value));
        }
    }
    return data;
}

inline auto readParmAtomName(std::ifstream &parm, const size_t number)
    -> std::vector<std::string> {
    auto removeNonAlpha = [](const std::string &input) -> std::string {
        std::string output;
        for (const auto &ch : input) {
            if (std::isalpha(static_cast<unsigned char>(ch))) { output += ch; }
        }
        return output;
    };

    std::vector<std::string> data;
    std::string line;
    while (std::getline(parm, line)) {
        if (line.find("%FLAG ATOM_NAME") != std::string::npos) {
            std::getline(parm, line);
            break;
        }
    }
    while (data.size() < number and std::getline(parm, line)) {
        for (size_t i = 0; i < line.size() / 4; ++i) {
            data.emplace_back(removeNonAlpha(line.substr(4 * i, 4)));
        }
    }
    return data;
}

inline auto readPrmtop(const std::string &parm_path) -> FFParm {
    std::ifstream parm(parm_path);
    std::string line;

    size_t n_atom, n_type;
    while (std::getline(parm, line)) {
        if (line.find("%FLAG POINTERS") != std::string::npos) {
            std::getline(parm, line);
            std::getline(parm, line);
            std::istringstream iss(line);
            iss >> n_atom;
            iss >> n_type;
            break;
        }
    }

    std::vector<std::string> atom_name = readParmAtomName(parm, n_atom);

    std::vector<double> charges =
        readParm<double>(parm, n_atom, "%FLAG CHARGE");
    for (auto &charge : charges) {
        charge = charge / Factor_au2amber;
    }

    const size_t n_atom_type = n_atom;
    std::vector<int> atom_type_index =
        readParm<int>(parm, n_atom_type, "%FLAG ATOM_TYPE_INDEX");

    const size_t n_nonbonded = n_type * n_type;
    std::vector<int> nonbonded_parm_index =
        readParm<int>(parm, n_nonbonded, "%FLAG NONBONDED_PARM_INDEX");

    const size_t n_lj_acoef = n_type * (n_type + 1) / 2;
    std::vector<double> lj_acoef =
        readParm<double>(parm, n_lj_acoef, "%FLAG LENNARD_JONES_ACOEF");

    const size_t n_lj_bcoef = n_type * (n_type + 1) / 2;
    std::vector<double> lj_bcoef =
        readParm<double>(parm, n_lj_bcoef, "%FLAG LENNARD_JONES_BCOEF");

    return {n_atom,   n_type,          atom_name,    charges,
            atom_type_index, nonbonded_parm_index, lj_acoef, lj_bcoef};
}

inline auto readRestart(const std::string &restart, const size_t atom_number)
    -> std::vector<double> {
    std::ifstream rst(restart);
    std::string line;
    std::vector<double> data;
    data.reserve(3 * atom_number + 3);

    std::getline(rst, line);
    std::getline(rst, line);
    for (size_t i = 0; i < 3 * atom_number + 3; ++i) {
        double _data;
        rst >> _data;
        data.push_back(std::move(_data));
    }
    return data;
}

inline auto readTraj(const std::string &trajactory, const size_t atom_number,
                     const size_t frame_number)
    -> std::vector<std::vector<double>> {
    std::ifstream traj(trajactory);
    std::string line;
    std::vector<std::vector<double>> data;
    data.resize(frame_number);

    std::getline(traj, line);
    for (size_t i = 0; i < frame_number; ++i) {
        data[i].reserve(3 * atom_number + 3);
        for (size_t j = 0; j < 3 * atom_number + 3; ++j) {
            double _data;
            traj >> _data;
            data[i].push_back(std::move(_data));
        }
    }
    return data;
}

inline auto atomSquareDistance(const double x1, const double y1, const double z1,
                               const double x2, const double y2, const double z2)
    -> double {
    const double dx = x1 - x2;
    const double dy = y1 - y2;
    const double dz = z1 - z2;
    return dx * dx + dy * dy + dz * dz;
}

inline auto eCoul(const double r2inv, const double q1, const double q2) -> double {
    return q1 * q2 * std::sqrt(r2inv);
}

inline auto getLJCoeff34(const FFParm &parm, const size_t p1, const size_t p2) -> std::pair<double, double> {
    const double iacj = parm.n_type * (parm.atom_type_index[p1] - 1);
    const double ic =
        parm.nonbonded_parm_index[iacj + parm.atom_type_index[p2] - 1];
    return {parm.lj_acoef[ic - 1], parm.lj_bcoef[ic - 1]};
}

inline auto eVdwl(const double r2inv, const double lj3, const double lj4) -> double {
    const double r6inv = r2inv * r2inv * r2inv;
    return r6inv * (lj3 * r6inv - lj4);
}

} // namespace amber
} // namespace meika
#endif
