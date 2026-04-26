#pragma once
#ifndef MEIKA_ENGRAD
#define MEIKA_ENGRAD

#include "base.h"
#include "system.h"

#include <string>

#include <fstream>
#include <sstream>

namespace meika {
namespace engrad {
auto readEngrad(const std::string &file) -> system::System;

inline auto readEngrad(const std::string &file) -> system::System {
    std::ifstream engrad(file);
    std::string line;

    int num_atoms = 0;
    while (std::getline(engrad, line)) {
        if (line.find("# Number of atoms") != std::string::npos) {
            std::getline(engrad, line);
            std::getline(engrad, line);
            num_atoms = std::stoi(line);
            break;
        }
    }

    system::System sys(num_atoms, system::System::InitMode::Resize);

    while (std::getline(engrad, line)) {
        if (line.find("# The current total energy in Eh") !=
            std::string::npos) {
            std::getline(engrad, line);
            std::getline(engrad, line);
            sys.ep = meika::toReal(line.c_str());
            break;
        }
    }

    while (std::getline(engrad, line)) {
        if (line.find("# The current gradient in Eh/bohr") !=
            std::string::npos) {
            std::getline(engrad, line);
            break;
        }
    }

    int count = 0;
    while (count < sys.n * 3) {
        std::getline(engrad, line);
        const real val = meika::toReal(line.c_str());
        const int idx = count / 3;
        switch (count % 3) {
        case 0:
            sys.gx[idx] = val;
        case 1:
            sys.gy[idx] = val;
        case 2:
            sys.gz[idx] = val;
        default:
            break;
        }
        count++;
    }

    while (std::getline(engrad, line)) {
        if (line.find("# The atomic numbers and current coordinates in Bohr") !=
            std::string::npos) {
            std::getline(engrad, line);
            break;
        }
    }

    count = 0;
    while (count < sys.n) {
        std::getline(engrad, line);
        const char *p = line.c_str();
        char *endptr;

        int type_ = meika::toInt(p, &endptr);
        if (p == endptr) continue;
        p = endptr;

        real x_ = meika::toReal(p, &endptr);
        if (p == endptr) continue;
        p = endptr;
        real y_ = meika::toReal(p, &endptr);
        if (p == endptr) continue;
        p = endptr;
        real z_ = meika::toReal(p, &endptr);
        if (p == endptr) continue;
        p = endptr;

        sys.type[count] = type_;
        sys.element[count] = system::atomicNumber2Element(type_);
        sys.x[count] = x_;
        sys.y[count] = y_;
        sys.z[count] = z_;
        count++;
    }

    return sys;
}
} // namespace engrad
} // namespace meika
#endif
