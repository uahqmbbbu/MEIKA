#pragma once

#include "pdb.h"
#include "xyz.h"

#include "app-state.h"
#include "command.h"

#include <fstream>
#include <iostream>

// Helper: get topo by 1-based id
inline meika::system::Topology *getTopo(AppState &st, const std::string &id_str) {
    size_t id = std::stoull(id_str);
    if (id < 1 || id > st.topologies.size()) return nullptr;
    return &st.topologies[id - 1];
}

// Helper: apply atom range to Topology
inline meika::system::Topology applyAtomRange(const meika::system::Topology &src,
                                                const std::vector<int> &idx) {
    meika::system::Topology dst(idx.size(), meika::system::InitMode::Resize);
    dst.n = idx.size();
    for (size_t j = 0; j < idx.size(); ++j) {
        int i = idx[j];
        dst.idx[j]     = j;
        dst.name[j]    = src.name[i];
        dst.type[j]    = src.type[i];
        dst.element[j] = src.element[i];
        dst.chain[j]   = src.chain[i];
        dst.res_name[j]= src.res_name[i];
        dst.res_idx[j] = src.res_idx[i];
        dst.ter_res[j] = src.ter_res[i];
        dst.ter_idx[j] = src.ter_idx[i];
        dst.occupancy[j]   = src.occupancy[i];
        dst.temp_factor[j] = src.temp_factor[i];
        dst.charge[j]  = src.charge[i];
        dst.epsilon[j] = src.epsilon[i];
        dst.sigma[j]   = src.sigma[i];
    }
    // expair is based on atom pairs — can't easily crop, keep empty
    return dst;
}

// ─── topo load ────────────────────────────────────────────────────────────

inline void cmdTopoLoad(AppState &st, const std::vector<std::string> &args) {
    size_t id = std::stoull(args[1]);
    const auto &fn = args[2];
    auto fmt = detectFmt(fn);

    meika::system::System tmp;
    try {
        if (fmt == "pdb") tmp = meika::pdb::readPDB(fn);
        else if (fmt == "xyz") tmp = meika::xyz::readXYZ(fn);
        else { std::cout << "! Unknown format: " << fn << "\n"; return; }
    } catch (const std::exception &e) { std::cout << "! " << e.what() << "\n"; return; }

    auto topo = tmp.topology();
    // Check for atoms param
    if (args.size() >= 5 && args[3] == "atoms") {
        auto idx = parseIndices(args[4], topo.n);
        topo = applyAtomRange(topo, idx);
    }

    while (st.topologies.size() < id) { st.topologies.emplace_back(); st.topo_origins.push_back(""); }
    st.topologies[id - 1] = std::move(topo);
    st.topo_origins[id - 1] = fn;
    std::cout << "topo[" << id << "] = " << st.topologies[id - 1].n << " atoms <- " << fn << "\n";
}

// ─── topo copy ────────────────────────────────────────────────────────────

inline void cmdTopoCopy(AppState &st, const std::vector<std::string> &args) {
    size_t id = std::stoull(args[1]);
    size_t src = std::stoull(args[3]);
    auto *t = getTopo(st, args[3]);
    if (!t) { std::cout << "! topo[" << src << "] not found\n"; return; }
    while (st.topologies.size() < id) { st.topologies.emplace_back(); st.topo_origins.push_back(""); }
    st.topologies[id - 1] = *t;
    st.topo_origins[id - 1] = "copy of topo[" + std::to_string(src) + "]";
    std::cout << "topo[" << id << "] = " << t->n << " atoms <- topo[" << src << "]\n";
}

// ─── topo extract ─────────────────────────────────────────────────────────

inline void cmdTopoExtract(AppState &st, const std::vector<std::string> &args) {
    size_t id = std::stoull(args[1]);
    if (args.size() < 4 || args[2] != "system") { std::cout << "! usage: topo extract <id> system <sys_id> [atoms <range>]\n"; return; }
    size_t sid = std::stoull(args[3]);
    if (sid < 1 || sid > st.systems.size()) { std::cout << "! system[" << sid << "] not found\n"; return; }

    auto topo = st.systems[sid - 1].topology();
    if (args.size() >= 6 && args[4] == "atoms") {
        auto idx = parseIndices(args[5], topo.n);
        topo = applyAtomRange(topo, idx);
    }

    while (st.topologies.size() < id) { st.topologies.emplace_back(); st.topo_origins.push_back(""); }
    st.topologies[id - 1] = std::move(topo);
    st.topo_origins[id - 1] = "from system[" + std::to_string(sid) + "]";
    std::cout << "topo[" << id << "] = " << st.topologies[id - 1].n << " atoms <- system[" << sid << "]\n";
}

// ─── topo list ────────────────────────────────────────────────────────────

inline void cmdTopoList(AppState &st, const std::vector<std::string> &) {
    for (size_t i = 0; i < st.topologies.size(); ++i)
        std::cout << "topo[" << (i+1) << "] " << st.topologies[i].n << " atoms <- " << st.topo_origins[i] << "\n";
    if (st.topologies.empty()) std::cout << "(no topologies)\n";
}

// ─── topo show ────────────────────────────────────────────────────────────

inline void cmdTopoShow(AppState &st, const std::vector<std::string> &args) {
    auto *t = getTopo(st, args[1]);
    if (!t) { std::cout << "! topo[" << args[1] << "] not found\n"; return; }
    std::cout << "topo[" << args[1] << "] " << t->n << " atoms <- " << st.topo_origins[std::stoull(args[1])-1] << "\n";
}

// ─── topo output ──────────────────────────────────────────────────────────

inline void cmdTopoOutput(AppState &st, const std::vector<std::string> &args) {
    auto *t = getTopo(st, args[1]);
    if (!t) { std::cout << "! topo[" << args[1] << "] not found\n"; return; }
    const auto &fn = args[2];

    const meika::system::Topology *src = t;
    meika::system::Topology cropped;
    if (args.size() >= 5 && args[3] == "atoms") {
        auto idx = parseIndices(args[4], t->n);
        cropped = applyAtomRange(*t, idx);
        src = &cropped;
    }

    meika::system::System tmp(*src, meika::system::State(src->n, meika::system::InitMode::Resize));
    std::ofstream out(fn);
    meika::pdb::outputPDB(out, tmp);
    std::cout << "Written: " << fn << "\n";
}

// ─── topo delete ──────────────────────────────────────────────────────────

inline void cmdTopoDelete(AppState &st, const std::vector<std::string> &args) {
    size_t id = std::stoull(args[1]);
    if (id < 1 || id > st.topologies.size()) { std::cout << "! topo[" << id << "] not found\n"; return; }
    st.topologies.erase(st.topologies.begin() + (id - 1));
    st.topo_origins.erase(st.topo_origins.begin() + (id - 1));
    std::cout << "Deleted topo[" << id << "]\n";
}

// ─── dispatcher ───────────────────────────────────────────────────────────

inline bool dispatchTopo(AppState &st, const std::vector<std::string> &args) {
    if (args.empty() || args[0] != "topo") return false;
    if (args.size() < 2) { std::cout << "! usage: topo <verb> ...\n"; return true; }
    const auto &verb = args[1];

    if (verb == "load" && args.size() >= 3) cmdTopoLoad(st, args);
    else if (verb == "copy" && args.size() >= 4) cmdTopoCopy(st, args);
    else if (verb == "extract" && args.size() >= 4) cmdTopoExtract(st, args);
    else if (verb == "list") cmdTopoList(st, args);
    else if (verb == "show" && args.size() >= 3) cmdTopoShow(st, args);
    else if (verb == "output" && args.size() >= 3) cmdTopoOutput(st, args);
    else if (verb == "delete" && args.size() >= 3) cmdTopoDelete(st, args);
    else std::cout << "! topo: unknown verb or wrong args\n";
    return true;
}
