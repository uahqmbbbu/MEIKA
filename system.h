#pragma once
#ifndef MEIKA_SYSTEM
#define MEIKA_SYSTEM

#include "base.h"
#include "span.h"

#include <array>
#include <string>
#include <unordered_map>
#include <vector>

#include <iomanip>
#include <iostream>

namespace meika {
namespace system {

enum class InitMode { Reserve, Resize };

class Topology {
  public:
    int n;

    std::vector<int> idx;
    std::vector<std::string> name;
    std::vector<int> type;
    std::vector<std::string> element;

    std::vector<std::string> chain;
    std::vector<std::string> res_name;
    std::vector<int> res_idx;
    std::vector<int> ter_res;
    std::vector<int> ter_idx;

    std::vector<real> occupancy;
    std::vector<real> temp_factor;
    std::vector<real> charge;
    std::vector<real> epsilon;
    std::vector<real> sigma;

    std::vector<std::array<int, 2>> expair;
    std::vector<real> expair_charge_prod;
    std::vector<real> expair_epsilon;
    std::vector<real> expair_sigma;

    Topology();
    Topology(int num, InitMode mode);
};

class State {
  public:
    real ep;
    real ek;
    real et;

    std::vector<real> x;
    std::vector<real> y;
    std::vector<real> z;
    std::vector<real> gx;
    std::vector<real> gy;
    std::vector<real> gz;
    std::vector<real> vx;
    std::vector<real> vy;
    std::vector<real> vz;

    bool pbc = false;
    std::array<real, 6> cell{};

    State();
    State(int num, InitMode mode);
};

class System {
  public:
    int n;
    real ep;
    real ek;
    real et;

    std::vector<int> idx;
    std::vector<std::string> name;
    std::vector<int> type;
    std::vector<std::string> element;

    std::vector<std::string> chain;
    std::vector<std::string> res_name;
    std::vector<int> res_idx;
    std::vector<int> ter_res;
    std::vector<int> ter_idx;

    std::vector<real> x;
    std::vector<real> y;
    std::vector<real> z;
    std::vector<real> gx;
    std::vector<real> gy;
    std::vector<real> gz;
    std::vector<real> vx;
    std::vector<real> vy;
    std::vector<real> vz;

    std::vector<real> occupancy;
    std::vector<real> temp_factor;
    std::vector<real> charge;
    std::vector<real> epsilon;
    std::vector<real> sigma;

    std::vector<std::array<int, 2>> expair;
    std::vector<real> expair_charge_prod;
    std::vector<real> expair_epsilon;
    std::vector<real> expair_sigma;

    bool pbc = false;
    std::array<real, 6> cell{};

    using InitMode = meika::system::InitMode;
    System();
    System(const int num, const InitMode mode);
    System(const Topology &topo, const State &state);
    ~System() = default;

    auto topology() const -> Topology;
    auto state() const -> State;

    template <typename T, typename M>
    auto extractMember(const std::vector<T> &vec,
                       M T::*member) -> std::vector<M> {
        std::vector<M> out;
        out.reserve(vec.size());
        for (const auto &x : vec) { out.push_back(x.*member); }
        return out;
    }

    template <typename T>
    auto unitTransform(T &data, const double factor) -> void {
        data *= factor;
    }

    auto unitTransform(std::vector<real> &data, const double factor) -> void {
#pragma omp parallel for
        for (size_t i = 0; i < data.size(); ++i) { data[i] *= factor; }
    }

    auto unitTransform(std::vector<real> &x, std::vector<real> &y,
                       std::vector<real> &z, const double factor) -> void {
#pragma omp parallel for
        for (size_t i = 0; i < x.size(); ++i) {
            x[i] *= factor;
            y[i] *= factor;
            z[i] *= factor;
        }
    }
};

auto atomicNumber2Element(const int type) -> std::string;
auto element2AtomicNumber(const std::string &element) -> int;

inline System::System() {}
inline System::System(const int num, const InitMode mode) {
    n = num;
    ep = 0.0;
    ek = 0.0;
    et = 0.0;
    pbc = false;
    cell = {};

#define BATCH_APPLY(op)                                                        \
    idx.op(n);                                                                 \
    name.op(n);                                                                \
    type.op(n);                                                                \
    element.op(n);                                                             \
    chain.op(n);                                                               \
    res_name.op(n);                                                            \
    res_idx.op(n);                                                             \
    x.op(num);                                                                 \
    y.op(num);                                                                 \
    z.op(num);                                                                 \
    gx.op(num);                                                                \
    gy.op(num);                                                                \
    gz.op(num);                                                                \
    vx.op(num);                                                                \
    vy.op(num);                                                                \
    vz.op(num);                                                                  \
    occupancy.op(n);                                                           \
    temp_factor.op(n);                                                         \
    charge.op(n);                                                              \
    epsilon.op(n);                                                             \
    sigma.op(n);

    if (mode == InitMode::Reserve) {
        BATCH_APPLY(reserve);
    } else if (mode == InitMode::Resize) {
        BATCH_APPLY(resize);
    }

#undef BATCH_APPLY //
}

inline Topology::Topology() {}
inline Topology::Topology(int num, InitMode mode) {
    n = num;

#define BATCH_TOPO(op)                                                         \
    idx.op(n);                                                                 \
    name.op(n);                                                                \
    type.op(n);                                                                \
    element.op(n);                                                             \
    chain.op(n);                                                               \
    res_name.op(n);                                                            \
    res_idx.op(n);                                                             \
    ter_res.op(n);                                                             \
    ter_idx.op(n);                                                             \
    occupancy.op(n);                                                           \
    temp_factor.op(n);                                                         \
    charge.op(n);                                                              \
    epsilon.op(n);                                                             \
    sigma.op(n);                                                               \
    expair.op(n);                                                              \
    expair_charge_prod.op(n);                                                  \
    expair_epsilon.op(n);                                                      \
    expair_sigma.op(n);

    if (mode == InitMode::Reserve) {
        BATCH_TOPO(reserve);
    } else if (mode == InitMode::Resize) {
        BATCH_TOPO(resize);
    }

#undef BATCH_TOPO
}

inline State::State() {}
inline State::State(int num, InitMode mode) {
    ep = 0.0;
    ek = 0.0;
    et = 0.0;
    pbc = false;
    cell = {};

#define BATCH_STATE(op)                                                        \
    x.op(num);                                                                 \
    y.op(num);                                                                 \
    z.op(num);                                                                 \
    gx.op(num);                                                                \
    gy.op(num);                                                                \
    gz.op(num);                                                                \
    vx.op(num);                                                                \
    vy.op(num);                                                                \
    vz.op(num);

    if (mode == InitMode::Reserve) {
        BATCH_STATE(reserve);
    } else if (mode == InitMode::Resize) {
        BATCH_STATE(resize);
    }

#undef BATCH_STATE
}

inline System::System(const Topology &topo, const State &state) {
    n = topo.n;
    ep = state.ep;
    ek = state.ek;
    et = state.et;
    pbc = state.pbc;
    cell = state.cell;

    idx = topo.idx;
    name = topo.name;
    type = topo.type;
    element = topo.element;
    chain = topo.chain;
    res_name = topo.res_name;
    res_idx = topo.res_idx;
    ter_res = topo.ter_res;
    ter_idx = topo.ter_idx;
    occupancy = topo.occupancy;
    temp_factor = topo.temp_factor;
    charge = topo.charge;
    epsilon = topo.epsilon;
    sigma = topo.sigma;
    expair = topo.expair;
    expair_charge_prod = topo.expair_charge_prod;
    expair_epsilon = topo.expair_epsilon;
    expair_sigma = topo.expair_sigma;

    x = state.x;
    y = state.y;
    z = state.z;
    gx = state.gx;
    gy = state.gy;
    gz = state.gz;
    vx = state.vx;
    vy = state.vy;
    vz = state.vz;
}

inline auto System::topology() const -> Topology {
    Topology t;
    t.n = n;
    t.idx = idx;
    t.name = name;
    t.type = type;
    t.element = element;
    t.chain = chain;
    t.res_name = res_name;
    t.res_idx = res_idx;
    t.ter_res = ter_res;
    t.ter_idx = ter_idx;
    t.occupancy = occupancy;
    t.temp_factor = temp_factor;
    t.charge = charge;
    t.epsilon = epsilon;
    t.sigma = sigma;
    t.expair = expair;
    t.expair_charge_prod = expair_charge_prod;
    t.expair_epsilon = expair_epsilon;
    t.expair_sigma = expair_sigma;
    return t;
}

inline auto System::state() const -> State {
    State s;
    s.ep = ep;
    s.ek = ek;
    s.et = et;
    s.x = x;
    s.y = y;
    s.z = z;
    s.gx = gx;
    s.gy = gy;
    s.gz = gz;
    s.vx = vx;
    s.vy = vy;
    s.vz = vz;
    s.pbc = pbc;
    s.cell = cell;
    return s;
}

inline auto atomicNumber2Element(const int type) -> std::string {
    switch (type) {
    case 1:
        return "H";
    case 2:
        return "He";
    case 3:
        return "Li";
    case 4:
        return "Be";
    case 5:
        return "B";
    case 6:
        return "C";
    case 7:
        return "N";
    case 8:
        return "O";
    case 9:
        return "F";
    case 10:
        return "Ne";
    case 11:
        return "Na";
    case 12:
        return "Mg";
    case 13:
        return "Al";
    case 14:
        return "Si";
    case 15:
        return "P";
    case 16:
        return "S";
    case 17:
        return "Cl";
    case 26:
        return "Fe";
    default:
        return "X";
    };
}

inline const std::unordered_map<std::string, int> &
get_name_to_atomic_number_map() {
    static const std::unordered_map<std::string, int> data = {
        {"H", 1},    {"He", 2},   {"Li", 3},   {"Be", 4},   {"B", 5},
        {"C", 6},    {"N", 7},    {"O", 8},    {"F", 9},    {"Ne", 10},
        {"Na", 11},  {"Mg", 12},  {"Al", 13},  {"Si", 14},  {"P", 15},
        {"S", 16},   {"Cl", 17},  {"Ar", 18},  {"K", 19},   {"Ca", 20},
        {"Sc", 21},  {"Ti", 22},  {"V", 23},   {"Cr", 24},  {"Mn", 25},
        {"Fe", 26},  {"Co", 27},  {"Ni", 28},  {"Cu", 29},  {"Zn", 30},
        {"Ga", 31},  {"Ge", 32},  {"As", 33},  {"Se", 34},  {"Br", 35},
        {"Kr", 36},  {"Rb", 37},  {"Sr", 38},  {"Y", 39},   {"Zr", 40},
        {"Nb", 41},  {"Mo", 42},  {"Tc", 43},  {"Ru", 44},  {"Rh", 45},
        {"Pd", 46},  {"Ag", 47},  {"Cd", 48},  {"In", 49},  {"Sn", 50},
        {"Sb", 51},  {"Te", 52},  {"I", 53},   {"Xe", 54},  {"Cs", 55},
        {"Ba", 56},  {"La", 57},  {"Ce", 58},  {"Pr", 59},  {"Nd", 60},
        {"Pm", 61},  {"Sm", 62},  {"Eu", 63},  {"Gd", 64},  {"Tb", 65},
        {"Dy", 66},  {"Ho", 67},  {"Er", 68},  {"Tm", 69},  {"Yb", 70},
        {"Lu", 71},  {"Hf", 72},  {"Ta", 73},  {"W", 74},   {"Re", 75},
        {"Os", 76},  {"Ir", 77},  {"Pt", 78},  {"Au", 79},  {"Hg", 80},
        {"Tl", 81},  {"Pb", 82},  {"Bi", 83},  {"Po", 84},  {"At", 85},
        {"Rn", 86},  {"Fr", 87},  {"Ra", 88},  {"Ac", 89},  {"Th", 90},
        {"Pa", 91},  {"U", 92},   {"Np", 93},  {"Pu", 94},  {"Am", 95},
        {"Cm", 96},  {"Bk", 97},  {"Cf", 98},  {"Es", 99},  {"Fm", 100},
        {"Md", 101}, {"No", 102}, {"Lr", 103}, {"Rf", 104}, {"Db", 105},
        {"Sg", 106}, {"Bh", 107}, {"Hs", 108}, {"Mt", 109}, {"Ds", 110},
        {"Rg", 111}, {"Cn", 112}, {"Nh", 113}, {"Fl", 114}, {"Mc", 115},
        {"Lv", 116}, {"Ts", 117}, {"Og", 118}};
    return data;
}

inline auto element2AtomicNumber(const std::string &element) -> int {
    const auto &table = get_name_to_atomic_number_map();
    auto it = table.find(element);
    if (it != table.end()) { return it->second; }
    return 0; // 或者抛出异常
}
} // namespace system
} // namespace meika
#endif
