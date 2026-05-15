#pragma once

#include "dcd.h"
#include "xyz.h"

#include "app-state.h"
#include "command.h"

#include <fstream>
#include <iostream>

// ─── helpers ──────────────────────────────────────────────────────────────

inline meika::system::Topology getTopoFromRef(AppState &st, const std::string &type, const std::string &id_str) {
    size_t ref = std::stoull(id_str);
    if (type == "topo") {
        if (ref < 1 || ref > st.topologies.size())
            throw std::runtime_error("topo[" + id_str + "] not found");
        return st.topologies[ref - 1];
    } else { // system
        if (ref < 1 || ref > st.systems.size())
            throw std::runtime_error("system[" + id_str + "] not found");
        return st.systems[ref - 1].topology();
    }
}

// ─── states load ──────────────────────────────────────────────────────────

inline void cmdStatesLoad(AppState &st, const std::vector<std::string> &args) {
    // args: load <id> <file> [dcd <topo|system> <ref_id> | xyz]
    size_t id = std::stoull(args[1]);
    const auto &fn = args[2];

    std::vector<meika::system::State> states;
    std::string origin = fn;

    if (args.size() >= 4 && args[3] == "dcd") {
        // states load <id> <file> dcd <topo|system> <ref_id>
        if (args.size() < 6) { std::cout << "! usage: states load <id> <file> dcd <topo|system> <ref_id>\n"; return; }
        auto topo = getTopoFromRef(st, args[4], args[5]);
        try {
            states = meika::dcd::readDCD(fn, topo);
        } catch (const std::exception &e) { std::cout << "! " << e.what() << "\n"; return; }
    } else {
        // states load <id> <file> [xyz] — multi-frame XYZ
        try {
            auto frames = meika::xyz::readMultiFrameXYZ(fn);
            states.reserve(frames.size());
            for (auto &f : frames) states.push_back(f.state());
        } catch (const std::exception &e) { std::cout << "! " << e.what() << "\n"; return; }
    }

    while (st.states.size() < id) { st.states.emplace_back(); st.states_origins.push_back(""); }
    st.states[id - 1] = std::move(states);
    st.states_origins[id - 1] = origin;
    std::cout << "states[" << id << "] = " << st.states[id - 1].size() << " frames <- " << origin << "\n";
}

// ─── states copy ──────────────────────────────────────────────────────────

inline void cmdStatesCopy(AppState &st, const std::vector<std::string> &args) {
    size_t id = std::stoull(args[1]);
    size_t src = std::stoull(args[3]);
    if (src < 1 || src > st.states.size()) { std::cout << "! states[" << src << "] not found\n"; return; }

    while (st.states.size() < id) { st.states.emplace_back(); st.states_origins.push_back(""); }
    st.states[id - 1] = st.states[src - 1];
    st.states_origins[id - 1] = "copy of states[" + std::to_string(src) + "]";
    std::cout << "states[" << id << "] = " << st.states[id - 1].size() << " frames <- states[" << src << "]\n";
}

// ─── states extract ───────────────────────────────────────────────────────

inline void cmdStatesExtract(AppState &st, const std::vector<std::string> &args) {
    size_t id = std::stoull(args[1]);
    size_t sid = std::stoull(args[3]);
    if (sid < 1 || sid > st.systems.size()) { std::cout << "! system[" << sid << "] not found\n"; return; }

    std::vector<meika::system::State> states;
    states.push_back(st.systems[sid - 1].state());
    while (st.states.size() < id) { st.states.emplace_back(); st.states_origins.push_back(""); }
    st.states[id - 1] = std::move(states);
    st.states_origins[id - 1] = "from system[" + std::to_string(sid) + "]";
    std::cout << "states[" << id << "] = 1 frame <- system[" << sid << "]\n";
}

// ─── states list ──────────────────────────────────────────────────────────

inline void cmdStatesList(AppState &st, const std::vector<std::string> &) {
    for (size_t i = 0; i < st.states.size(); ++i)
        std::cout << "states[" << (i+1) << "] " << st.states[i].size() << " frames <- " << st.states_origins[i] << "\n";
    if (st.states.empty()) std::cout << "(no states)\n";
}

// ─── states show ──────────────────────────────────────────────────────────

inline void cmdStatesShow(AppState &st, const std::vector<std::string> &args) {
    size_t id = std::stoull(args[1]);
    if (id < 1 || id > st.states.size()) { std::cout << "! states[" << id << "] not found\n"; return; }
    const auto &ss = st.states[id - 1];
    std::cout << "states[" << id << "] " << ss.size() << " frames <- " << st.states_origins[id - 1] << "\n";
    if (!ss.empty()) std::cout << "  atoms: " << ss[0].x.size() << ", pbc: " << (ss[0].pbc ? "true" : "false") << "\n";
}

// ─── states output ────────────────────────────────────────────────────────

inline void cmdStatesOutput(AppState &st, const std::vector<std::string> &args) {
    // args: output <id> <file> <dcd|xyz> <topo|system> <ref_id> [frames <range>] [atoms <range>]
    size_t id = std::stoull(args[1]);
    if (id < 1 || id > st.states.size()) { std::cout << "! states[" << id << "] not found\n"; return; }
    const auto &fn = args[2];
    const auto &fmt = args[3];

    // Get name source
    std::string name_type; size_t name_ref = 0;
    if (args.size() >= 6) { name_type = args[4]; name_ref = std::stoull(args[5]); }

    // Parse optional frames/atoms
    std::vector<int> frame_idx;
    std::vector<int> atom_idx;
    for (size_t i = 6; i + 1 < args.size(); i++) {
        if (args[i] == "frames") { frame_idx = parseIndices(args[i + 1], st.states[id - 1].size()); i++; }
        else if (args[i] == "atoms") {
            size_t natoms = st.states[id - 1].empty() ? 0 : st.states[id - 1][0].x.size();
            atom_idx = parseIndices(args[i + 1], natoms); i++;
        }
    }

    // Build name list
    std::vector<std::string> names;
    if (name_ref > 0) {
        if (name_type == "topo" && name_ref <= st.topologies.size())
            names = st.topologies[name_ref - 1].name;
        else if (name_type == "system" && name_ref <= st.systems.size())
            names = st.systems[name_ref - 1].name;
    }
    // Apply atom selection to names
    if (!atom_idx.empty() && !names.empty()) {
        std::vector<std::string> sel;
        for (auto i : atom_idx) if (i >= 0 && (size_t)i < names.size()) sel.push_back(names[i]);
        names = std::move(sel);
    }

    // Determine frames to output
    std::vector<size_t> frames_out;
    if (frame_idx.empty()) {
        for (size_t fi = 0; fi < st.states[id - 1].size(); ++fi) frames_out.push_back(fi);
    } else {
        for (auto fi : frame_idx) if (fi >= 0 && (size_t)fi < (int)st.states[id - 1].size()) frames_out.push_back(fi);
    }

    if (fmt == "dcd") {
        std::vector<meika::system::State> out;
        for (auto fi : frames_out) out.push_back(st.states[id - 1][fi]);
        std::ofstream o(fn, std::ios::binary);
        meika::dcd::outputDCD(o, out.size(), meika::span<meika::system::State>(out));
        std::cout << "Written: " << fn << " (" << out.size() << " frames)\n";
    } else if (fmt == "xyz") {
        if (names.empty()) { names.resize(st.states[id - 1][0].x.size()); for (auto &n : names) n = "X"; }
        for (auto fi : frames_out) {
            const auto &s = st.states[id - 1][fi];
            std::ofstream o(fn + std::to_string(fi + 1) + ".xyz");
            // Apply atom selection
            std::vector<std::string> ns = names;
            std::vector<meika::real> xs, ys, zs;
            if (atom_idx.empty()) {
                xs = s.x; ys = s.y; zs = s.z;
            } else {
                for (auto ai : atom_idx) {
                    if (ai >= 0 && (size_t)ai < s.x.size()) {
                        xs.push_back(s.x[ai]); ys.push_back(s.y[ai]); zs.push_back(s.z[ai]);
                    }
                }
            }
            meika::xyz::outputXYZ(o, ns, xs, ys, zs, 15, 7);
        }
        std::cout << "Written: " << frames_out.size() << " frames to " << fn << "*.xyz\n";
    } else {
        std::cout << "! Unknown format: " << fmt << "\n";
    }
}

// ─── states delete ────────────────────────────────────────────────────────

inline void cmdStatesDelete(AppState &st, const std::vector<std::string> &args) {
    size_t id = std::stoull(args[1]);
    if (id < 1 || id > st.states.size()) { std::cout << "! states[" << id << "] not found\n"; return; }
    st.states.erase(st.states.begin() + (id - 1));
    st.states_origins.erase(st.states_origins.begin() + (id - 1));
    std::cout << "Deleted states[" << id << "]\n";
}

// ─── states slice ─────────────────────────────────────────────────────────

inline void cmdStatesSlice(AppState &st, const std::vector<std::string> &args) {
    size_t id = std::stoull(args[1]);
    size_t src = std::stoull(args[2]);
    if (src < 1 || src > st.states.size()) { std::cout << "! states[" << src << "] not found\n"; return; }
    auto idx = parseIndices(args[3], st.states[src - 1].size());
    std::vector<meika::system::State> sel;
    for (auto i : idx) if (i >= 0 && (size_t)i < st.states[src - 1].size()) sel.push_back(st.states[src - 1][i]);

    while (st.states.size() < id) { st.states.emplace_back(); st.states_origins.push_back(""); }
    st.states[id - 1] = std::move(sel);
    st.states_origins[id - 1] = "slice of states[" + std::to_string(src) + "] " + args[3];
    std::cout << "states[" << id << "] = " << st.states[id - 1].size() << " frames <- " << st.states_origins[id - 1] << "\n";
}

// ─── dispatcher ───────────────────────────────────────────────────────────

inline bool dispatchStates(AppState &st, const std::vector<std::string> &args) {
    if (args.empty() || args[0] != "states") return false;
    if (args.size() < 2) { std::cout << "! usage: states <verb> ...\n"; return true; }
    const auto &verb = args[1];

    if (verb == "load" && args.size() >= 3) cmdStatesLoad(st, args);
    else if (verb == "copy" && args.size() >= 4) cmdStatesCopy(st, args);
    else if (verb == "extract" && args.size() >= 4) cmdStatesExtract(st, args);
    else if (verb == "list") cmdStatesList(st, args);
    else if (verb == "show" && args.size() >= 3) cmdStatesShow(st, args);
    else if (verb == "output" && args.size() >= 5) cmdStatesOutput(st, args);
    else if (verb == "delete" && args.size() >= 3) cmdStatesDelete(st, args);
    else if (verb == "slice" && args.size() >= 4) cmdStatesSlice(st, args);
    else std::cout << "! states: unknown verb or wrong args\n";
    return true;
}
