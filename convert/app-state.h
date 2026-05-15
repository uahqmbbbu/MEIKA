#pragma once

#include "system.h"

#include <iostream>
#include <vector>

struct AppState {
    using System = meika::system::System;
    using Topology = meika::system::Topology;
    using State = meika::system::State;
    using real = meika::real;
    std::vector<System> systems;
    std::vector<Topology> topologies;
    std::vector<std::vector<State>> states;

    std::vector<std::string> system_origins;
    std::vector<std::string> topo_origins;
    std::vector<std::string> states_origins;

    enum class ObjectType { System, Topology, State };
    size_t ObjectCount(const ObjectType type) const {
        return type == ObjectType::System     ? systems.size()
               : type == ObjectType::Topology ? topologies.size()
                                              : states.size();
    }

    size_t addSystem(const System &sys, const std::string &origin) {
        systems.emplace_back(sys);
        system_origins.push_back(origin);
        return systems.size();
    }

    size_t addSystem(System &&sys, const std::string &origin) {
        systems.emplace_back(std::move(sys));
        system_origins.push_back(origin);
        return systems.size();
    }

    size_t addTopology(const Topology &topo, const std::string &origin) {
        topologies.emplace_back(topo);
        topo_origins.push_back(origin);
        return topologies.size();
    }

    size_t addTopology(Topology &&topo, const std::string &origin) {
        topologies.emplace_back(std::move(topo));
        topo_origins.push_back(origin);
        return topologies.size();
    }

    size_t addStates(const std::vector<State> &st, const std::string &origin) {
        states.emplace_back(st);
        states_origins.push_back(origin);
        return states.size();
    }

    size_t addStates(std::vector<State> &&st, const std::string &origin) {
        states.emplace_back(std::move(st));
        states_origins.push_back(origin);
        return states.size();
    }
};
