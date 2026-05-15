#pragma once

#include "engrad.h"
#include "pdb.h"
#include "xyz.h"
#include "unit.h"

#include "app-state.h"
#include "command.h"

#include <fstream>
#include <iomanip>
#include <iostream>

// Helper: get system by 1-based id, nullptr if invalid
inline meika::system::System *getSys(AppState &st, const std::string &id_str) {
    size_t id = std::stoull(id_str);
    if (id < 1 || id > st.systems.size()) return nullptr;
    return &st.systems[id - 1];
}

// Helper: apply atom range to a System, return new System
inline meika::system::System applyAtomRange(const meika::system::System &src,
                                              const std::vector<int> &idx) {
    meika::system::System dst(idx.size(), meika::system::InitMode::Resize);
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
        dst.x[j] = src.x[i]; dst.y[j] = src.y[i]; dst.z[j] = src.z[i];
        if (i < (int)src.gx.size()) { dst.gx[j] = src.gx[i]; dst.gy[j] = src.gy[i]; dst.gz[j] = src.gz[i]; }
        if (i < (int)src.vx.size()) { dst.vx[j] = src.vx[i]; dst.vy[j] = src.vy[i]; dst.vz[j] = src.vz[i]; }
        dst.occupancy[j]   = src.occupancy[i];
        dst.temp_factor[j] = src.temp_factor[i];
        dst.charge[j]  = src.charge[i];
        dst.epsilon[j] = src.epsilon[i];
        dst.sigma[j]   = src.sigma[i];
    }
    dst.ep = src.ep; dst.ek = src.ek; dst.et = src.et;
    dst.pbc = src.pbc; dst.cell = src.cell;
    return dst;
}

// Helper: parse optional atoms parameter
inline std::vector<int> parseAtomsOpt(const std::vector<std::string> &args, size_t pos, int natoms) {
    if (pos + 1 < args.size() && args[pos] == "atoms")
        return parseIndices(args[pos + 1], natoms);
    return {};
}

// ─── system load ──────────────────────────────────────────────────────────

inline void cmdSystemLoad(AppState &st, const std::vector<std::string> &args) {
    // args: load <id> <file> [atoms <range>]
    size_t id = std::stoull(args[1]);
    const auto &fn = args[2];
    auto fmt = detectFmt(fn);

    meika::system::System sys;
    try {
        if (fmt == "pdb") sys = meika::pdb::readPDB(fn);
        else if (fmt == "xyz") sys = meika::xyz::readXYZ(fn);
        else if (fmt == "engrad") sys = meika::engrad::readEngrad(fn);
        else { std::cout << "! Unknown format: " << fn << "\n"; return; }
    } catch (const std::exception &e) {
        std::cout << "! Error loading: " << e.what() << "\n"; return;
    }

    auto atoms = parseAtomsOpt(args, 3, sys.n);
    if (!atoms.empty()) sys = applyAtomRange(sys, atoms);

    // Ensure slot exists
    while (st.systems.size() < id) {
        st.systems.emplace_back(); st.system_origins.push_back("");
    }
    st.systems[id - 1] = std::move(sys);
    st.system_origins[id - 1] = fn;
    std::cout << "system[" << id << "] = " << st.systems[id - 1].n
              << " atoms <- " << fn << "\n";
}

// ─── system copy ──────────────────────────────────────────────────────────

inline void cmdSystemCopy(AppState &st, const std::vector<std::string> &args) {
    // args: copy <id> system <src_id>
    size_t id = std::stoull(args[1]);
    size_t src = std::stoull(args[3]);
    auto *s = getSys(st, args[3]);
    if (!s) { std::cout << "! system[" << src << "] not found\n"; return; }

    while (st.systems.size() < id) {
        st.systems.emplace_back(); st.system_origins.push_back("");
    }
    st.systems[id - 1] = *s;
    st.system_origins[id - 1] = "copy of system[" + std::to_string(src) + "]";
    std::cout << "system[" << id << "] = " << s->n << " atoms <- system[" << src << "]\n";
}

// ─── system build ─────────────────────────────────────────────────────────

inline void cmdSystemBuild(AppState &st, const std::vector<std::string> &args) {
    // args: build <id> topo <t_id> states <s_id> <frame>
    size_t id = std::stoull(args[1]);
    size_t tid = std::stoull(args[3]);
    size_t sid = std::stoull(args[5]);
    size_t frame = std::stoull(args[6]);

    if (tid < 1 || tid > st.topologies.size()) { std::cout << "! topo[" << tid << "] not found\n"; return; }
    if (sid < 1 || sid > st.states.size()) { std::cout << "! states[" << sid << "] not found\n"; return; }
    if (frame >= st.states[sid - 1].size()) { std::cout << "! frame " << frame << " out of range\n"; return; }

    meika::system::System sys(st.topologies[tid - 1], st.states[sid - 1][frame]);

    while (st.systems.size() < id) {
        st.systems.emplace_back(); st.system_origins.push_back("");
    }
    st.systems[id - 1] = std::move(sys);
    st.system_origins[id - 1] = "topo[" + std::to_string(tid) + "] + states[" + std::to_string(sid) + "][" + std::to_string(frame) + "]";
    std::cout << "system[" << id << "] = " << st.systems[id - 1].n
              << " atoms <- " << st.system_origins[id - 1] << "\n";
}

// ─── system list ──────────────────────────────────────────────────────────

inline void cmdSystemList(AppState &st, const std::vector<std::string> &) {
    for (size_t i = 0; i < st.systems.size(); ++i)
        std::cout << "system[" << (i + 1) << "] " << st.systems[i].n
                  << " atoms <- " << st.system_origins[i] << "\n";
    if (st.systems.empty()) std::cout << "(no systems)\n";
}

// ─── system show ──────────────────────────────────────────────────────────

inline void cmdSystemShow(AppState &st, const std::vector<std::string> &args) {
    auto *s = getSys(st, args[1]);
    if (!s) { std::cout << "! system[" << args[1] << "] not found\n"; return; }
    std::cout << "system[" << args[1] << "] " << s->n << " atoms, ep=" << s->ep
              << ", pbc=" << (s->pbc ? "true" : "false") << "\n";
    std::cout << "  origin: " << st.system_origins[std::stoull(args[1]) - 1] << "\n";
}

// ─── system output ────────────────────────────────────────────────────────

inline void cmdSystemOutput(AppState &st, const std::vector<std::string> &args) {
    // args: output <id> <file> <fmt> [atoms <range>]
    auto *s = getSys(st, args[1]);
    if (!s) { std::cout << "! system[" << args[1] << "] not found\n"; return; }
    const auto &fn = args[2];
    const auto &fmt = args[3];
    auto atoms = parseAtomsOpt(args, 4, s->n);

    const meika::system::System *src = s;
    meika::system::System cropped;
    if (!atoms.empty()) { cropped = applyAtomRange(*s, atoms); src = &cropped; }

    const auto &sys = *src;

    if (fmt == "pdb") {
        std::ofstream out(fn);
        meika::pdb::outputPDB(out, sys);
    } else if (fmt == "xyz") {
        if (fn == "-") meika::xyz::outputXYZ(std::cout, sys.name, sys.x, sys.y, sys.z, 15, 7);
        else { std::ofstream out(fn); meika::xyz::outputXYZ(out, sys.name, sys.x, sys.y, sys.z, 15, 7); }
    } else if (fmt == "mace") {
        std::ostream *o; std::ofstream fo;
        if (fn == "-") o = &std::cout; else { fo.open(fn); o = &fo; }
        *o << std::fixed << sys.n << "\n";
        *o << "Properties=species:S:1:pos:R:3:REF_forces:R:3 REF_energy="
           << std::setprecision(12) << sys.ep << " pbc=\"F F F\"\n";
        for (int i = 0; i < sys.n; ++i)
            *o << std::setw(2) << std::left << sys.element[i] << " "
               << std::right << std::setw(15) << std::setprecision(7) << sys.x[i] << " "
               << std::setw(15) << std::setprecision(7) << sys.y[i] << " "
               << std::setw(15) << std::setprecision(7) << sys.z[i] << " "
               << std::setw(20) << std::setprecision(12) << -sys.gx[i] << " "
               << std::setw(20) << std::setprecision(12) << -sys.gy[i] << " "
               << std::setw(20) << std::setprecision(12) << -sys.gz[i] << "\n";
        *o << std::flush;
    } else if (fmt == "deepmd") {
        auto tt = [](int t)->int{switch(t){case 6:return 0;case 1:return 1;case 7:return 2;case 8:return 3;case 15:return 4;default:return -1;}};
        {std::ofstream o("type.raw");for(int i=0;i<sys.n;++i)o<<tt(sys.type[i])<<"\n";}
        {std::ofstream o("coord.raw",std::ios::app);o<<std::fixed;for(int i=0;i<sys.n;++i)o<<std::setw(15)<<std::setprecision(7)<<sys.x[i]<<" "<<std::setw(15)<<std::setprecision(7)<<sys.y[i]<<" "<<std::setw(15)<<std::setprecision(7)<<sys.z[i]<<" ";o<<"\n";}
        {std::ofstream o("energy.raw",std::ios::app);o<<std::fixed<<std::setprecision(12)<<sys.ep*meika::unit::HATREE2EV<<"\n";}
        {std::ofstream o("force.raw",std::ios::app);o<<std::fixed;for(int i=0;i<sys.n;++i)o<<std::setw(20)<<std::setprecision(12)<<-sys.gx[i]*meika::unit::HATREE_BOHR2EV_ANGSTROM<<" "<<std::setw(20)<<std::setprecision(12)<<-sys.gy[i]*meika::unit::HATREE_BOHR2EV_ANGSTROM<<" "<<std::setw(20)<<std::setprecision(12)<<-sys.gz[i]*meika::unit::HATREE_BOHR2EV_ANGSTROM<<" ";o<<"\n";}
        std::cout << "Written: type.raw coord.raw energy.raw force.raw\n";
        return;
    } else { std::cout << "! Unknown format: " << fmt << "\n"; return; }
    std::cout << "Written: " << fn << "\n";
}

// ─── system delete ────────────────────────────────────────────────────────

inline void cmdSystemDelete(AppState &st, const std::vector<std::string> &args) {
    size_t id = std::stoull(args[1]);
    if (id < 1 || id > st.systems.size()) { std::cout << "! system[" << id << "] not found\n"; return; }
    st.systems.erase(st.systems.begin() + (id - 1));
    st.system_origins.erase(st.system_origins.begin() + (id - 1));
    std::cout << "Deleted system[" << id << "]\n";
}

// ─── system transform ─────────────────────────────────────────────────────

inline void cmdSystemTransform(AppState &st, const std::vector<std::string> &args) {
    auto *s = getSys(st, args[1]);
    if (!s) { std::cout << "! system[" << args[1] << "] not found\n"; return; }
    const auto &t = args[2];
    if (t == "a2b")      { s->unitTransform(s->x,s->y,s->z,meika::unit::ANGSTROM2BOHR); }
    else if (t == "b2a") { s->unitTransform(s->x,s->y,s->z,meika::unit::BOHR2ANGSTROM); }
    else if (t == "h2ev"){ s->unitTransform(s->ep,meika::unit::HATREE2EV); }
    else if (t == "ev2h"){ s->unitTransform(s->ep,meika::unit::EV2HATREE); }
    else if (t == "hb2ea"){
        s->unitTransform(s->ep,meika::unit::HATREE2EV);
        s->unitTransform(s->x,s->y,s->z,meika::unit::BOHR2ANGSTROM);
        s->unitTransform(s->gx,s->gy,s->gz,meika::unit::HATREE_BOHR2EV_ANGSTROM);
    } else if (t == "ea2hb"){
        s->unitTransform(s->ep,meika::unit::EV2HATREE);
        s->unitTransform(s->x,s->y,s->z,meika::unit::ANGSTROM2BOHR);
        s->unitTransform(s->gx,s->gy,s->gz,meika::unit::EV_ANGSTROM2HATREE_BOHR);
    } else { std::cout << "! Unknown transform: " << t << "\n"; return; }
    std::cout << "system[" << args[1] << "] transformed: " << t << "\n";
}

// ─── system d4 ────────────────────────────────────────────────────────────

inline void cmdSystemD4(AppState &st, const std::vector<std::string> &args) {
    auto *s = getSys(st, args[1]);
    if (!s) { std::cout << "! system[" << args[1] << "] not found\n"; return; }
    std::ifstream f(args[2]);
    if (!f) { std::cout << "! Cannot open: " << args[2] << "\n"; return; }

    std::string line; double d4e=0;
    std::vector<std::array<double,3>> d4g; bool rd=false;
    while (std::getline(f,line)) {
        if (line.find("Dispersion correction")!=std::string::npos && line.find("... done")==std::string::npos) {
            std::stringstream ss(line); std::string a,b; ss>>a>>b>>d4e;
        }
        if (line.find("DISPERSION GRADIENT")!=std::string::npos) { rd=true; std::getline(f,line); std::getline(f,line); continue; }
        if (rd) {
            if (line.empty()||line.find("Difference to")!=std::string::npos) { rd=false; continue; }
            std::stringstream ss(line); int idx; std::string el,colon; double gx,gy,gz;
            if (ss>>idx>>el>>colon>>gx>>gy>>gz) d4g.push_back({gx,gy,gz});
        }
    }
    s->ep -= d4e;
    for (int i=0; i<s->n && (size_t)i<d4g.size(); ++i) {
        s->gx[i]-=d4g[i][0]; s->gy[i]-=d4g[i][1]; s->gz[i]-=d4g[i][2];
    }
    std::cout << "system[" << args[1] << "] D4 subtracted: dE=" << d4e << "\n";
}

// ─── system update-coords ─────────────────────────────────────────────────

inline void cmdSystemUpdateCoords(AppState &st, const std::vector<std::string> &args) {
    auto *s = getSys(st, args[1]);
    if (!s) { std::cout << "! system[" << args[1] << "] not found\n"; return; }
    auto fmt = detectFmt(args[2]);
    meika::system::System coord;
    try {
        if (fmt == "engrad") coord = meika::engrad::readEngrad(args[2]);
        else coord = meika::xyz::readXYZ(args[2]);
    } catch (const std::exception &e) { std::cout << "! " << e.what() << "\n"; return; }
    if (coord.n != s->n) { std::cout << "! Atom count mismatch\n"; return; }
    s->x = std::move(coord.x); s->y = std::move(coord.y); s->z = std::move(coord.z);
    std::cout << "system[" << args[1] << "] coords updated from " << args[2] << "\n";
}

// ─── system change-state ──────────────────────────────────────────────────

inline void cmdSystemChangeState(AppState &st, const std::vector<std::string> &args) {
    auto *s = getSys(st, args[1]);
    if (!s) { std::cout << "! system[" << args[1] << "] not found\n"; return; }
    size_t sid = std::stoull(args[2]);
    size_t frame = std::stoull(args[3]);
    if (sid < 1 || sid > st.states.size()) { std::cout << "! states[" << sid << "] not found\n"; return; }
    if (frame >= st.states[sid - 1].size()) { std::cout << "! frame out of range\n"; return; }
    const auto &state = st.states[sid - 1][frame];
    if (state.x.size() != (size_t)s->n) { std::cout << "! Atom count mismatch\n"; return; }
    for (int i = 0; i < s->n; ++i) { s->x[i]=state.x[i]; s->y[i]=state.y[i]; s->z[i]=state.z[i]; }
    if (!state.gx.empty()) for(int i=0;i<s->n;++i){s->gx[i]=state.gx[i];s->gy[i]=state.gy[i];s->gz[i]=state.gz[i];}
    if (!state.vx.empty()) for(int i=0;i<s->n;++i){s->vx[i]=state.vx[i];s->vy[i]=state.vy[i];s->vz[i]=state.vz[i];}
    s->pbc = state.pbc; s->cell = state.cell;
    std::cout << "system[" << args[1] << "] state changed to states[" << sid << "][" << frame << "]\n";
}

// ─── dispatcher ───────────────────────────────────────────────────────────

inline bool dispatchSystem(AppState &st, const std::vector<std::string> &args) {
    if (args.empty() || args[0] != "system") return false;
    if (args.size() < 2) { std::cout << "! usage: system <verb> ...\n"; return true; }
    const auto &verb = args[1];

    if (verb == "load" && args.size() >= 3) cmdSystemLoad(st, args);
    else if (verb == "copy" && args.size() >= 4) cmdSystemCopy(st, args);
    else if (verb == "build" && args.size() >= 7) cmdSystemBuild(st, args);
    else if (verb == "list") cmdSystemList(st, args);
    else if (verb == "show" && args.size() >= 3) cmdSystemShow(st, args);
    else if (verb == "output" && args.size() >= 4) cmdSystemOutput(st, args);
    else if (verb == "delete" && args.size() >= 3) cmdSystemDelete(st, args);
    else if (verb == "transform" && args.size() >= 4) cmdSystemTransform(st, args);
    else if (verb == "d4" && args.size() >= 4) cmdSystemD4(st, args);
    else if (verb == "update-coords" && args.size() >= 4) cmdSystemUpdateCoords(st, args);
    else if (verb == "change-state" && args.size() >= 5) cmdSystemChangeState(st, args);
    else std::cout << "! system: unknown verb or wrong args\n";
    return true;
}
