#pragma once
#ifndef MEIKA_UNIT
#define MEIKA_UNIT

#include "base.h"

namespace meika {
namespace unit {
// 物理常数
constexpr real CONST_AVOGADRO = 6.02214076e23; // mol^-1

// 长度
constexpr real BOHR2ANGSTROM = 0.529177210544;
constexpr real ANGSTROM2BOHR = 1.0 / BOHR2ANGSTROM;

constexpr real NANOMETER2ANGSTROM = 10.0000000;
constexpr real ANGSTROM2NANOMETER = 0.1000000;

constexpr real BOHR2NANOMETER = BOHR2ANGSTROM * ANGSTROM2NANOMETER;
constexpr real NANOMETER2BOHR = NANOMETER2ANGSTROM * ANGSTROM2BOHR;

constexpr real CENTIMETER2NANOMETER = 1e7;
constexpr real NANOMETER2CENTIMETER = 1 / CENTIMETER2NANOMETER;

// 能量单位
constexpr real HATREE2J = 4.3597447222060e-18;
constexpr real EV2J = 1.602176634e-19;
constexpr real KCAL2KJ = 4.184;

constexpr real HATREE2EV = 27.211386245981;
constexpr real EV2HATREE = 1.0 / HATREE2EV;

constexpr real HATREE2KJ = HATREE2J * CONST_AVOGADRO / 1000;
constexpr real EV2KJ = EV2J * CONST_AVOGADRO / 1000;

constexpr real HATREE2KCAL = HATREE2KJ / KCAL2KJ;
constexpr real EV2KCAL = EV2KJ / KCAL2KJ;

// 力单位：Hatree/Bohr
constexpr real HATREE_BOHR2EV_ANGSTROM = HATREE2EV / BOHR2ANGSTROM;
constexpr real EV_ANGSTROM2HATREE_BOHR = EV2HATREE * BOHR2ANGSTROM;
constexpr real EV_ANGSTROM2KJ_NANOMETER = EV2KJ * NANOMETER2ANGSTROM;
constexpr real KJ_NANOMETER2EV_ANGSTROM = 1 / EV_ANGSTROM2KJ_NANOMETER;
} // namespace unit
} // namespace meika
#endif
