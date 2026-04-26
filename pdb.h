#pragma once
#ifndef MEIKA_PDB
#define MEIKA_PDB

#include "base.h"
#include "span.h"
#include "system.h"

#include <set>
#include <string>
#include <vector>

#include <cctype>
#include <fstream>

namespace meika {
namespace pdb {
auto readPDB(span<const std::string> pdb) -> system::System;
auto readPDB(const std::string &filename) -> system::System;

auto outputPDB(std::ostream &buffer, const system::System &sys) -> void;

inline auto readPDB(span<const std::string> pdb) -> system::System {
    system::System sys(pdb.size(), system::System::InitMode::Reserve);
    int num_atoms = 0;
    for (std::size_t i = 0; i < pdb.size(); ++i) {
        const std::string &line = pdb[i];
        const char *const p = line.c_str();

        if (line.compare(0, 4, "ATOM") == 0 or
            line.compare(0, 6, "HETATM") == 0) {
            const int index = num_atoms++;
            sys.idx.emplace_back(index);

            auto trim_ptr =
                [](const char *start,
                   const int width) -> std::pair<const char *, std::size_t> {
                const char *end = start + width;
                // 剔除左侧空格
                while (start < end &&
                       std::isspace(static_cast<unsigned char>(*start)))
                    start++;
                // 剔除右侧空格
                while (end > start &&
                       std::isspace(static_cast<unsigned char>(*(end - 1))))
                    end--;
                return {start, static_cast<std::size_t>(end - start)};
            };

            std::pair<const char *, std::size_t> sub = {nullptr, 0};

            sub = trim_ptr(p + 12, 4);
            sys.name.emplace_back(sub.first, sub.second);

            sub = trim_ptr(p + 17, 3);
            sys.res_name.emplace_back(sub.first, sub.second);

            sys.chain.emplace_back(1, p[21]);

            sys.res_idx.emplace_back(toInt(p + 22, 4));

            sys.x.emplace_back(toReal(p + 30, 8));
            sys.y.emplace_back(toReal(p + 38, 8));
            sys.z.emplace_back(toReal(p + 46, 8));

            sys.occupancy.emplace_back(toReal(p + 54, 6));
            sys.temp_factor.emplace_back(toReal(p + 60, 6));

            auto removeNonAlpha = [](const std::string &input) -> std::string {
                std::string output;
                for (const auto &ch : input) {
                    if (std::isalpha(static_cast<unsigned char>(ch))) {
                        output += ch;
                    }
                }
                return output;
            };

            if (line.size() < 79) {
                sys.element.emplace_back(removeNonAlpha(sys.name.back()));
            } else {
                sub = trim_ptr(p + 76, 2);
                sys.element.emplace_back(sub.first, sub.second);
            }
        }
        if (line.compare(0, 3, "TER") == 0) {
            sys.ter_idx.emplace_back(num_atoms);
            sys.ter_res.emplace_back(toInt(p + 22, 4));
        }
    }
    sys.n = num_atoms;
    return sys;
}

inline auto readPDB(const std::string &filename) -> system::System {
    std::ifstream pdb(filename);
    if (!pdb.is_open()) {
        std::cerr << "Error: Unable to open file: " << filename << std::endl;
    }
    std::vector<std::string> data;
    std::string line;
    while (std::getline(pdb, line)) { data.push_back(std::move(line)); }
    return readPDB(span<const std::string>(data));
}

inline auto outputPDB(std::ostream &buffer, const system::System &sys) -> void {

    auto formatAtomName = [](const std::string &name,
                             const std::string &element) -> std::string {
        std::string formatted = "    ";
        // 元素符号在13-14列中右对齐，如果双字母元素，则直接占13-14列
        if (name.size() >= 4) {
            formatted = name.substr(0, 4);
        } else if (name.size() > 0 and name.size() < 4) {
            if (std::isdigit(name[0])) {
                // 如果以数字开头，元素符号在14列
                for (std::size_t i = 0; i < name.size() and i < 4; ++i)
                    formatted[i] = name[i];
            } else {
                // 根据元素字符判断：
                if (element.size() == 1) {
                    for (std::size_t i = 0; i < name.size() and i < 3; ++i)
                        formatted[i + 1] = name[i];
                } else {
                    for (std::size_t i = 0; i < name.size() and i < 4; ++i)
                        formatted[i] = name[i];
                }
            }
        }
        return formatted;
    };

    // 为输出"TER"
    auto ter_idx_set = std::set<int>(sys.ter_idx.begin(), sys.ter_idx.end());

    buffer << std::fixed;
    for (int i = 0; i < sys.n; ++i) {
        if (ter_idx_set.count(sys.idx[i])) {
            buffer << std::left << std::setw(6) << "TER";
            buffer << std::right << std::setw(5) << sys.idx[i] + 1 << " ";
            buffer << "     ";
            buffer << std::right << std::setw(3) << sys.res_name[i - 1] << " ";
            buffer << std::right << std::setw(1) << sys.chain[i];
            buffer << std::right << std::setw(4) << sys.res_idx[i - 1] << " \n";
        }
        buffer << std::left << std::setw(6) << "ATOM";
        buffer << std::right << std::setw(5) << sys.idx[i] + 1 << " ";
        buffer << std::right << std::setw(4)
               << formatAtomName(sys.name[i], sys.element[i]) << " ";
        buffer << std::right << std::setw(3) << sys.res_name[i] << " ";
        buffer << std::right << std::setw(1) << sys.chain[i];
        buffer << std::right << std::setw(4) << sys.res_idx[i] << "    ";
        buffer << std::right << std::setw(8) << std::setprecision(3)
               << sys.x[i];
        buffer << std::right << std::setw(8) << std::setprecision(3)
               << sys.y[i];
        buffer << std::right << std::setw(8) << std::setprecision(3)
               << sys.z[i];
        buffer << std::right << std::setw(6) << std::setprecision(2)
               << sys.occupancy[i];
        buffer << std::right << std::setw(6) << std::setprecision(2)
               << sys.temp_factor[i];
        buffer << "      "
               << "    ";
        buffer << std::right << std::setw(2) << sys.element[i];
        buffer << "  \n";
    }
    buffer << std::left << std::setw(6) << "TER";
    buffer << std::right << std::setw(5) << sys.idx.back() + 2 << " ";
    buffer << "     ";
    buffer << std::right << std::setw(3) << sys.res_name.back() << " ";
    buffer << std::right << std::setw(1) << sys.chain.back();
    buffer << std::right << std::setw(4) << sys.res_idx.back() << " \n";
    buffer << "END   \n";
}
} // namespace pdb
} // namespace meika
#endif
