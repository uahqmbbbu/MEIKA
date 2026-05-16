#include "app-state.h"
#include "command.h"
#include "cmd-system.h"
#include "cmd-topo.h"
#include "cmd-states.h"

#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <io.h>
#define ISATTY _isatty
#define FILENO _fileno
#else
#include <unistd.h>
#define ISATTY isatty
#define FILENO fileno
#endif

static bool isTTY() { return ISATTY(FILENO(stdin)); }

// Parse range spec or expand "states <id>" to all frame indices
static std::vector<int> resolveRange(const std::string &spec, AppState &st) {
    auto tokens = parseTokens(spec);
    if (tokens.size() >= 2 && tokens[0] == "states") {
        size_t sid = std::stoull(tokens[1]);
        if (sid < 1 || sid > st.states.size()) return {};
        std::vector<int> idx;
        for (size_t i = 0; i < st.states[sid - 1].size(); ++i) idx.push_back(i);
        return idx;
    }
    // Standard range: 1:100, 1:100:2, 1,3,5 -- no max_count needed
    return parseIndices(spec, SIZE_MAX);
}

// Execute a single parsed command line
static void executeLine(AppState &st, const std::string &line) {
    auto tokens = parseTokens(line);
    if (tokens.empty()) return;
    if (dispatchSystem(st, tokens)) return;
    if (dispatchTopo(st, tokens)) return;
    if (dispatchStates(st, tokens)) return;
    std::cout << "! unknown: " << line << "\n";
}

// Execute a list of lines, with optional variable expansion
static void executeBlock(AppState &st, const std::vector<std::string> &lines,
                         const std::string &var = "", int val = 0) {
    for (const auto &line : lines) {
        std::string expanded = var.empty() ? line : expandVar(line, var, val);
        executeLine(st, expanded);
    }
}

// Read a for-block body (lines until "done")
static std::vector<std::string> collectBlock() {
    std::vector<std::string> lines;
    while (true) {
        std::string line;
        if (!std::getline(std::cin, line)) std::exit(0);
        line = trim(line);
        if (line == ":q") { std::cout << "Bye.\n"; std::exit(0); }
        if (line == "done") break;
        if (line.empty() || line[0] == '#') continue;
        lines.push_back(line);
    }
    return lines;
}

int main(int argc, char *argv[]) {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);

    AppState st;

    // Auto-load argv[1] if it's a file
    if (argc > 1) {
        std::string fn = argv[1];
        if (fn == "-h" || fn == "--help") {
            std::cout << R"(MEIKA Convert -- command language REPL

Usage: meika-convert [file] < script

If <file> is given, it is loaded as system[1] automatically.
Otherwise the REPL reads commands from stdin until EOF or :q.

Supported file formats: pdb, xyz, engrad, dcd

--- system -----------------------------------------------------------
  system load   <id> <file> [atoms <range>]
      Load a system from pdb/xyz/engrad file.

  system copy   <id> system <src_id>
      Duplicate an existing system to a new slot.

  system build  <id> topo <t_id> states <s_id> <frame>
      Construct a system from a topology + a state frame.

  system list
      List all loaded systems (id, atom count, origin).

  system show   <id>
      Show details of a system (n, ep, pbc, origin).

  system output <id> <file> <pdb|xyz|mace|deepmd> [atoms <range>]
      Write a system to file. deepmd writes type.raw, coord.raw etc.
      Use "-" as file for xyz/mace to write to stdout.

  system delete <id>
      Remove a system from memory.

  system transform <id> <a2b|b2a|h2ev|ev2h|hb2ea|ea2hb>
      Convert units. a=Angstrom, b=Bohr, h=Hartree, ev=electronvolt.
      hb2ea/ea2hb transform all coords + energy + gradients at once.

  system d4     <id> <d4_gradient_file>
      Subtract DFT-D4 dispersion correction from energy and gradients.

  system update-coords <id> <file>
      Replace coordinates (x,y,z) from an engrad or xyz file.

  system change-state <id> <states_id> <frame>
      Replace coords/velocities/forces from a state frame.

--- topo --------------------------------------------------------------
  topo load    <id> <file> [atoms <range>]
      Load topology from pdb/xyz file.

  topo copy    <id> topo <src_id>
      Duplicate an existing topology.

  topo extract <id> system <sys_id> [atoms <range>]
      Extract topology from a loaded system.

  topo list
      List all topologies.

  topo show    <id>
      Show topology details.

  topo output  <id> <file> [atoms <range>]
      Write topology as PDB file.

  topo delete  <id>
      Remove a topology.

--- states ------------------------------------------------------------
  states load    <id> <file> [dcd <topo|system> <ref_id>]
      Load trajectory from multi-frame XYZ or DCD file.
      DCD requires a topology or system reference for atom names.

  states copy    <id> states <src_id>
      Duplicate an existing states collection.

  states extract <id> system <sys_id>
      Extract a single-frame state from a system.

  states list
      List all states collections.

  states show    <id>
      Show states details (frames, atoms, pbc).

  states output  <id> <file> <dcd|xyz> <topo|system> <ref_id>
                 [frames <range>] [atoms <range>]
      Write trajectory. XYZ writes one file per frame (file1.xyz, ...).

  states delete  <id>
      Remove a states collection.

  states slice   <id> <src_id> <range>
      Extract selected frames into a new states collection.

--- control flow -----------------------------------------------------
  for <var> in <range> do
    ...
  done
      Loop over indices, expanding $var in the body.
      Example:  for i in 1:10:2 do ... done

  :q            Quit the REPL.
  # ...         Comment line (ignored).

--- range syntax -----------------------------------------------------
  1:100          indices 1 through 100 (1-based, inclusive)
  1:100:2        step by 2
  1,3,5,7        explicit list
  states <id>    all frame indices of a states collection
)";
            return 0;
        }
        auto fmt = detectFmt(fn);
        try {
            meika::system::System sys;
            if (fmt == "pdb") sys = meika::pdb::readPDB(fn);
            else if (fmt == "xyz") sys = meika::xyz::readXYZ(fn);
            else if (fmt == "engrad") sys = meika::engrad::readEngrad(fn);
            else { std::cout << "! Unknown format. Use .pdb .xyz .engrad\n"; return 1; }
            while (st.systems.size() < 1) { st.systems.emplace_back(); st.system_origins.push_back(""); }
            st.systems[0] = std::move(sys);
            st.system_origins[0] = fn;
            std::cout << "system[1] = " << st.systems[0].n << " atoms <- " << fn << "\n";
        } catch (const std::exception &e) {
            std::cout << "! " << e.what() << "\n";
            return 1;
        }
    }

    // Main REPL loop
    while (true) {
        if (isTTY()) std::cout << "> " << std::flush;
        std::string line;
        if (!std::getline(std::cin, line)) break;
        line = trim(line);
        if (line == ":q") { std::cout << "Bye.\n"; break; }
        if (line.empty() || line[0] == '#') continue;

        auto tokens = parseTokens(line);

        // for <var> in <range> do
        if (tokens.size() >= 4 && tokens[0] == "for" && tokens[2] == "in" && tokens.back() == "do") {
            std::string var = tokens[1];
            // Reconstruct range spec from tokens[3] to tokens[last-1]
            std::string range_spec;
            for (size_t i = 3; i < tokens.size() - 1; ++i) {
                if (i > 3) range_spec += " ";
                range_spec += tokens[i];
            }
            auto indices = resolveRange(range_spec, st);
            auto body = collectBlock();
            for (int val : indices) {
                executeBlock(st, body, var, val);
            }
            continue;
        }

        executeLine(st, line);
    }

    return 0;
}
