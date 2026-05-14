#pragma once
#include <ostream>
#ifndef MEIKA_DCD
#define MEIKA_DCD

#include "base.h"
#include "span.h"
#include "system.h"

#include <fstream>
#include <istream>
#include <vector>

namespace meika {
namespace dcd {
auto readDCD(std::istream &buffer,
            const system::Topology &topo) -> std::vector<system::State>;
auto readDCD(const std::string &filename,
             const system::Topology &topo) -> std::vector<system::State>;

auto outputDCD(std::ostream &buffer, const int nframes,
               const span<system::System> &systems) -> void;
auto outputDCD(std::ostream &buffer, const int nframes,
               const span<system::State> &states) -> void;

inline auto
readDCD(std::istream &buffer,
        const system::Topology &topo) -> std::vector<system::State> {
    int32_t marker;

    // Record 1: Control Info (84 bytes)
    buffer.read(reinterpret_cast<char *>(&marker), 4);

    char cord[4];
    buffer.read(cord, 4);

    int32_t icontrol[20] = {0};
    buffer.read(reinterpret_cast<char *>(icontrol), 80);

    buffer.read(reinterpret_cast<char *>(&marker), 4);

    bool has_unitcell = icontrol[10] != 0;

    // Record 2: Title Block (164 bytes)
    buffer.read(reinterpret_cast<char *>(&marker), 4);

    int32_t ntitle;
    buffer.read(reinterpret_cast<char *>(&ntitle), 4);

    buffer.seekg(160, std::ios::cur);

    buffer.read(reinterpret_cast<char *>(&marker), 4);

    // Record 3: Atom count (4 bytes)
    buffer.read(reinterpret_cast<char *>(&marker), 4);

    int32_t natoms;
    buffer.read(reinterpret_cast<char *>(&natoms), 4);

    buffer.read(reinterpret_cast<char *>(&marker), 4);

    // Frame data
    std::vector<system::State> states;
    std::vector<float> x(natoms), y(natoms), z(natoms);
    const int32_t coord_bytes = natoms * static_cast<int32_t>(sizeof(float));

    while (buffer.good() && !buffer.eof()) {
        double cell_data[6] = {0};
        bool frame_has_cell = false;

        if (has_unitcell) {
            buffer.read(reinterpret_cast<char *>(&marker), 4);
            if (!buffer.good()) break;
            if (marker == 48) {
                buffer.read(reinterpret_cast<char *>(cell_data), 48);
                buffer.read(reinterpret_cast<char *>(&marker), 4);
                if (!buffer.good()) break;
                frame_has_cell = true;
            } else {
                buffer.seekg(-4, std::ios::cur);
            }
        }

        auto read_coords = [&](std::vector<float> &coords) -> bool {
            buffer.read(reinterpret_cast<char *>(&marker), 4);
            if (!buffer.good()) return false;
            if (marker != coord_bytes) return false;
            buffer.read(reinterpret_cast<char *>(coords.data()), coord_bytes);
            buffer.read(reinterpret_cast<char *>(&marker), 4);
            return buffer.good();
        };

        if (!read_coords(x)) break;
        if (!read_coords(y)) break;
        if (!read_coords(z)) break;

        system::State s(natoms, system::InitMode::Resize);
        for (int i = 0; i < natoms; ++i) {
            s.x[i] = static_cast<real>(x[i]);
            s.y[i] = static_cast<real>(y[i]);
            s.z[i] = static_cast<real>(z[i]);
        }
        if (frame_has_cell) {
            s.pbc = true;
            for (int i = 0; i < 6; ++i) s.cell[i] = static_cast<real>(cell_data[i]);
        }
        states.push_back(std::move(s));
    }

    return states;
}

inline auto
readDCD(const std::string &filename,
        const system::Topology &topo) -> std::vector<system::State> {
    std::ifstream dcd(filename, std::ios::binary);
    if (!dcd.is_open()) {
        std::cerr << "Error: Unable to open file: " << filename << std::endl;
        return {};
    }
    return readDCD(dcd, topo);
}

inline auto outputDCD(std::ostream &buffer, const int nframes,
                      const span<system::System> &systems) -> void {

    const bool has_cell = systems[0].pbc;

    // Record 1: Control Info
    int32_t icontrol[20] = {0};
    icontrol[0] = nframes;
    icontrol[1] = 1;
    icontrol[2] = 1;
    icontrol[8] = 0;
    icontrol[10] = has_cell ? 1 : 0;
    icontrol[19] = 24;

    int32_t b84 = 84;
    buffer.write(reinterpret_cast<const char *>(&b84), 4);
    buffer.write("CORD", 4);
    buffer.write(reinterpret_cast<const char *>(icontrol), 80);
    buffer.write(reinterpret_cast<const char *>(&b84), 4);

    // Record 2: Title Block (2x80 chars)
    int32_t ntitle = 2;
    char title[160] = {0};
    std::snprintf(title, 80, "meika Created from XYZ file");
    std::snprintf(title + 80, 80, "Frame range applied");

    int32_t b164 = 164;
    buffer.write(reinterpret_cast<const char *>(&b164), 4);
    buffer.write(reinterpret_cast<const char *>(&ntitle), 4);
    buffer.write(title, 160);
    buffer.write(reinterpret_cast<const char *>(&b164), 4);

    // Record 3: Atoms
    int32_t natoms = static_cast<int32_t>(systems[0].n);
    int32_t b4 = 4;
    buffer.write(reinterpret_cast<const char *>(&b4), 4);
    buffer.write(reinterpret_cast<const char *>(&natoms), 4);
    buffer.write(reinterpret_cast<const char *>(&b4), 4);

    // Frames
    std::vector<float> x(natoms), y(natoms), z(natoms);
    double cell_data[6];
    for (const auto &sys : systems) {

        if (has_cell) {
            for (int i = 0; i < 6; ++i) cell_data[i] = static_cast<double>(sys.cell[i]);
            int32_t b48 = 48;
            buffer.write(reinterpret_cast<const char *>(&b48), 4);
            buffer.write(reinterpret_cast<const char *>(cell_data), 48);
            buffer.write(reinterpret_cast<const char *>(&b48), 4);
        }

        for (int i = 0; i < natoms; ++i) {
            x[i] = static_cast<float>(sys.x[i]);
            y[i] = static_cast<float>(sys.y[i]);
            z[i] = static_cast<float>(sys.z[i]);
        }

        auto write_rec = [&buffer](const void *data, std::size_t len) {
            int32_t b = static_cast<int32_t>(len);
            buffer.write(reinterpret_cast<const char *>(&b), 4);
            buffer.write(reinterpret_cast<const char *>(data), len);
            buffer.write(reinterpret_cast<const char *>(&b), 4);
        };

        std::size_t bytes = natoms * sizeof(float);
        write_rec(x.data(), bytes);
        write_rec(y.data(), bytes);
        write_rec(z.data(), bytes);
    }
}
inline auto outputDCD(std::ostream &buffer, const int nframes,
                      const span<system::State> &states) -> void {

    const bool has_cell = states[0].pbc;

    // Record 1: Control Info
    int32_t icontrol[20] = {0};
    icontrol[0] = nframes;
    icontrol[1] = 1;
    icontrol[2] = 1;
    icontrol[8] = 0;
    icontrol[10] = has_cell ? 1 : 0;
    icontrol[19] = 24;

    int32_t b84 = 84;
    buffer.write(reinterpret_cast<const char *>(&b84), 4);
    buffer.write("CORD", 4);
    buffer.write(reinterpret_cast<const char *>(icontrol), 80);
    buffer.write(reinterpret_cast<const char *>(&b84), 4);

    // Record 2: Title Block (2x80 chars)
    int32_t ntitle = 2;
    char title[160] = {0};
    std::snprintf(title, 80, "meika Created from XYZ file");
    std::snprintf(title + 80, 80, "Frame range applied");

    int32_t b164 = 164;
    buffer.write(reinterpret_cast<const char *>(&b164), 4);
    buffer.write(reinterpret_cast<const char *>(&ntitle), 4);
    buffer.write(title, 160);
    buffer.write(reinterpret_cast<const char *>(&b164), 4);

    // Record 3: Atoms
    int32_t natoms = static_cast<int32_t>(states[0].x.size());
    int32_t b4 = 4;
    buffer.write(reinterpret_cast<const char *>(&b4), 4);
    buffer.write(reinterpret_cast<const char *>(&natoms), 4);
    buffer.write(reinterpret_cast<const char *>(&b4), 4);

    // Frames
    std::vector<float> x(natoms), y(natoms), z(natoms);
    double cell_data[6];
    for (const auto &s : states) {

        if (has_cell) {
            for (int i = 0; i < 6; ++i) cell_data[i] = static_cast<double>(s.cell[i]);
            int32_t b48 = 48;
            buffer.write(reinterpret_cast<const char *>(&b48), 4);
            buffer.write(reinterpret_cast<const char *>(cell_data), 48);
            buffer.write(reinterpret_cast<const char *>(&b48), 4);
        }

        for (int i = 0; i < natoms; ++i) {
            x[i] = static_cast<float>(s.x[i]);
            y[i] = static_cast<float>(s.y[i]);
            z[i] = static_cast<float>(s.z[i]);
        }

        auto write_rec = [&buffer](const void *data, std::size_t len) {
            int32_t b = static_cast<int32_t>(len);
            buffer.write(reinterpret_cast<const char *>(&b), 4);
            buffer.write(reinterpret_cast<const char *>(data), len);
            buffer.write(reinterpret_cast<const char *>(&b), 4);
        };

        std::size_t bytes = natoms * sizeof(float);
        write_rec(x.data(), bytes);
        write_rec(y.data(), bytes);
        write_rec(z.data(), bytes);
    }
}
} // namespace dcd
} // namespace meika

#endif
