// Simulation engine for task execution on hosts using SimCpp20

#pragma once

#include <fschuetz04/simcpp20.hpp>
#include "container.hpp"
#include "models.h"
#include <memory>
#include <vector>
#include <unordered_map>
#include <map>
#include <string>

namespace simulator {

// Represents a compute host with CPU cores and RAM resources
struct Host {
    std::string name;
    simcpp20::resource<> cpu;
    simcpp20::container<> ram;
    int cpu_cores;
    int ram_capacity;

    Host(simcpp20::simulation<>& sim, const std::string& name,
         int cpu_cores, int ram_capacity);
};

using HostPtr = std::shared_ptr<Host>;

// Represents a full-duplex network link between hosts
class NetworkLink {
public:
    // Constructor takes list of host IDs to create links between all pairs
    NetworkLink(simcpp20::simulation<>& sim, const std::vector<std::string>& host_ids);

    // Get the appropriate network link for the given direction
    simcpp20::resource<>* get_link(const std::string& from_host,
                                   const std::string& to_host);

private:
    // Map from "from_host -> to_host" to network resource
    std::map<std::pair<std::string, std::string>, std::unique_ptr<simcpp20::resource<>>> links_;
};

using NetworkLinkPtr = std::shared_ptr<NetworkLink>;

// Task execution process (coroutine)
simcpp20::process<> task_process(
    simcpp20::simulation<>& sim,
    const models::Task& task,
    const std::unordered_map<std::string, HostPtr>& hosts,
    NetworkLinkPtr network,
    std::unordered_map<std::string, simcpp20::event<>>& task_completed,
    const std::unordered_map<std::string, models::Task>& task_dict);

// Main simulator for task execution
class TaskSimulator {
public:
    TaskSimulator(const models::ExperimentConfig& config,
                 const std::vector<models::Task>& tasks);

    // Run the simulation until all tasks complete
    void run(bool verbose = false);

private:
    simcpp20::simulation<> sim_;
    std::vector<models::Task> tasks_;
    std::unordered_map<std::string, models::Task> task_dict_;
    std::unordered_map<std::string, HostPtr> hosts_;
    NetworkLinkPtr network_;
    std::unordered_map<std::string, simcpp20::event<>> task_completed_;
};

} // namespace simulator
