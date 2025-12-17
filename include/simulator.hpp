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
    // Constructor takes number of hosts to create links between all pairs
    NetworkLink(simcpp20::simulation<>& sim, size_t num_hosts);

    // Get the appropriate network link for the given direction
    simcpp20::resource<>* get_link(size_t from_host_index, size_t to_host_index);

private:
    // Map from host index pair to network resource
    std::map<std::pair<size_t, size_t>, std::unique_ptr<simcpp20::resource<>>> links_;
};

using NetworkLinkPtr = std::shared_ptr<NetworkLink>;

// Task execution process (coroutine)
simcpp20::process<> task_process(
    simcpp20::simulation<>& sim,
    const models::Task& task,
    size_t task_index,
    const std::vector<HostPtr>& hosts,
    NetworkLinkPtr network,
    std::vector<simcpp20::event<>>& task_completed,
    const std::vector<models::Task>& tasks);

// Main simulator for task execution
class TaskSimulator {
public:
    TaskSimulator(const models::ExperimentConfig& config,
                 std::vector<models::Task>&& tasks);

    TaskSimulator() {}
    void init(const models::ExperimentConfig& config,
                 std::vector<models::Task>&& tasks);

    // Run the simulation until all tasks complete
    void run(bool verbose = false);

private:
    simcpp20::simulation<> sim_;
    std::vector<models::Task> tasks_;
    std::vector<HostPtr> hosts_;
    NetworkLinkPtr network_;
    std::vector<simcpp20::event<>> task_completed_;
    bool inited_ = false;
};

} // namespace simulator
