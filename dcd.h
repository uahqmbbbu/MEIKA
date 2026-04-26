#pragma once
#include <ostream>
#ifndef MEIKA_DCD
#define MEIKA_DCD

#include "base.h"
#include "span.h"
#include "system.h"

namespace meika {
namespace dcd {

auto outputDCD(std::ostream &buffer, const int nframes,
               const span<system::System> &systems) -> void;

inline auto outputDCD(std::ostream &buffer, const int nframes,
                      const span<system::System> &systems) -> void {

    // Record 1: Control Info
    int32_t icontrol[20] = {0};
    icontrol[0] = nframes; // 直接写入已知帧数
    icontrol[1] = 1;       // istart
    icontrol[2] = 1;       // nsavc
    icontrol[8] = 0;       // 自由原子数                           //
    icontrol[19] = 24;     // version

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
    for (const auto &sys : systems) {

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
} // namespace dcd
} // namespace meika

#endif
