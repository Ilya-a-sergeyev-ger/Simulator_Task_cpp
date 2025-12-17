#include "../include/simulator.hpp"
#include "../include/logger.hpp"
#include <iostream>
#include <stdexcept>
#include <sstream>

namespace simulator {

// Host implementation
Host::Host(simcpp20::simulation<>& sim, const std::string& name,
           int cpu_cores, int ram_capacity)
    : name(name),
      cpu(sim, cpu_cores),
      ram(sim, ram_capacity, ram_capacity), // container(sim, capacity, init_level)
      cpu_cores(cpu_cores),
      ram_capacity(ram_capacity) {

    logger::info("Host {} initialized: {} CPU cores, {} RAM units", name, cpu_cores, ram_capacity);
}

// NetworkLink implementation
NetworkLink::NetworkLink(simcpp20::simulation<>& sim, size_t num_hosts) {
    // Create full-duplex links between all pairs of hosts
    for (size_t from_idx = 0; from_idx < num_hosts; ++from_idx) {
        for (size_t to_idx = 0; to_idx < num_hosts; ++to_idx) {
            if (from_idx != to_idx) {
                auto key = std::make_pair(from_idx, to_idx);
                links_[key] = std::make_unique<simcpp20::resource<>>(sim, 1);
            }
        }
    }

    logger::info("Network initialized with {} directional links for {} hosts", links_.size(), num_hosts);
}

simcpp20::resource<>* NetworkLink::get_link(size_t from_host_index, size_t to_host_index) {
    auto key = std::make_pair(from_host_index, to_host_index);
    auto it = links_.find(key);

    if (it == links_.end()) {
        throw std::runtime_error("No network link from host " + std::to_string(from_host_index) +
                               " to host " + std::to_string(to_host_index));
    }

    return it->second.get();
}

// Task process coroutine
simcpp20::process<> task_process(
    simcpp20::simulation<>& sim,
    const models::Task& task,
    size_t task_index,
    const std::vector<HostPtr>& hosts,
    NetworkLinkPtr network,
    std::vector<simcpp20::event<>>& task_completed,
    const std::vector<models::Task>& tasks) {

    // Step 1: Initial sleep
    if (task.initial_sleep_time > 0) {
        logger::debug("[{}]\t[t={}]\tTask {}: Sleeping for {} time units",
                     task.host, static_cast<int>(sim.now()), task.name, task.initial_sleep_time);
        co_await sim.timeout(task.initial_sleep_time);
    }

    // Step 2: Wait for dependencies if exist
    for (size_t dep_index : task.dependency_indices) {
        const auto& dep_task = tasks[dep_index];

        logger::debug("[{}]\t[t={}]\tTask {}: Waiting for dependency {}",
                     task.host, static_cast<int>(sim.now()), task.name, dep_task.name);

        co_await task_completed[dep_index];

        // If cross-host dependency, wait for network transmission
        if (dep_task.host_index != task.host_index) {
            if (dep_task.network_time > 0) {
                auto* link = network->get_link(dep_task.host_index, task.host_index);

                logger::debug("[{}]\t[t={}]\tTask {}: Waiting for network transmission from {} ({} time units)",
                             task.host, static_cast<int>(sim.now()), task.name,
                             dep_task.name, dep_task.network_time);

                auto net_req = link->request();
                co_await net_req;

                logger::debug("[NETWORK]\t[t={}]\tTransmission started: {} -> {} ({} time units)",
                             static_cast<int>(sim.now()), dep_task.host, task.host, dep_task.network_time);

                co_await sim.timeout(dep_task.network_time);

                logger::debug("[NETWORK]\t[t={}]\tTransmission completed: {} -> {}",
                             static_cast<int>(sim.now()), dep_task.host, task.host);

                link->release();
            }
        }
    }

    // Step 3: Task is now ready
    logger::debug("[{}]\t[t={}]\tTask {}: Ready to execute",
                 task.host, static_cast<int>(sim.now()), task.name);

    // Step 4: Acquire resources (RAM and CPU)
    auto host = hosts[task.host_index];

    // Wait for available RAM (task will block until enough RAM is available)
    logger::debug("[{}]\t[t={}]\tTask {}: Waiting for {} RAM units",
                 task.host, static_cast<int>(sim.now()), task.name, task.ram);

    co_await host->ram.get(task.ram);

    // Wait for available CPU core
    logger::debug("[{}]\t[t={}]\tTask {}: Waiting for CPU core",
                 task.host, static_cast<int>(sim.now()), task.name);

    auto cpu_req = host->cpu.request();
    co_await cpu_req;

    logger::info("[{}]\t[t={}]\tTask {}: Started execution (CPU acquired, {} RAM allocated)",
                task.host, static_cast<int>(sim.now()), task.name, task.ram);

    // Step 5: Execute task (occupy CPU for run_time)
    co_await sim.timeout(task.run_time);

    logger::info("[{}]\t[t={}]\tTask {}: Finished execution",
                task.host, static_cast<int>(sim.now()), task.name);

    // Step 6: Release resources
    host->cpu.release();
    co_await host->ram.put(task.ram);

    logger::debug("[{}]\t[t={}]\tTask {}: Released {} RAM units",
                 task.host, static_cast<int>(sim.now()), task.name, task.ram);

    // Step 7: Mark task as completed, O(1) access
    task_completed[task_index].trigger();
}

// TaskSimulator implementation
TaskSimulator::TaskSimulator(const models::ExperimentConfig& config,
                            std::vector<models::Task>&& tasks)
    : tasks_(std::move(tasks)) {

    // Build task name to index mapping and resolve dependencies
    std::unordered_map<std::string, size_t> task_name_to_index;
    for (const auto& task : tasks_) {
        task_name_to_index[task.name] = task.index;
    }

    for (auto& task : tasks_) {
        for (const auto& dep_name : task.dependencies) {
            auto it = task_name_to_index.find(dep_name);
            if (it != task_name_to_index.end()) {
                task.dependency_indices.push_back(it->second);
            }
        }
    }

    // Create hosts and build host name to index mapping
    std::unordered_map<std::string, size_t> host_name_to_index;

    for (const auto& [host_id, host_config] : config.hosts) {
        size_t host_index = hosts_.size();
        hosts_.push_back(std::make_shared<Host>(
            sim_, host_id, host_config.cpu_cores, host_config.ram));
        host_name_to_index[host_id] = host_index;
    }

    // Convert task host names to indices and validate
    for (auto& task : tasks_) {
        auto it = host_name_to_index.find(task.host);
        if (it == host_name_to_index.end()) {
            throw std::runtime_error("Task '" + task.name + "' references unknown host: '" + task.host + "'");
        }
        task.host_index = it->second;
    }

    // Create network link
    network_ = std::make_shared<NetworkLink>(sim_, hosts_.size());

    // Create task completion events as a vector for O(1) access
    task_completed_.reserve(tasks_.size());
    for (size_t i = 0; i < tasks_.size(); ++i) {
        task_completed_.emplace_back(sim_.event());
    }
}

void TaskSimulator::run(bool verbose) {
    logger::info("======================================================================");
    logger::info("Starting simulation with {} tasks", tasks_.size());
    logger::info("======================================================================");

    // Calculate total CPU work time and per-host statistics
    int64_t total_cpu_work = 0;
    std::unordered_map<std::string, int64_t> cpu_work_per_host;

    for (const auto& task : tasks_) {
        total_cpu_work += task.run_time;
        cpu_work_per_host[task.host] += task.run_time;
    }

    // Schedule all tasks (start their coroutines)
    for (size_t i = 0; i < tasks_.size(); ++i) {
        task_process(sim_, tasks_[i], i, hosts_, network_, task_completed_, tasks_);
    }

    // Run simulation
    sim_.run();

    // Calculate metrics
    int64_t simulation_time = static_cast<int64_t>(sim_.now());

    // Calculate total available CPU time across all hosts
    int64_t total_cpu_cores = 0;
    for (const auto& host : hosts_) {
        total_cpu_cores += host->cpu_cores;
    }
    int64_t total_cpu_time_available = total_cpu_cores * simulation_time;

    // Calculate CPU utilization
    double cpu_utilization = (total_cpu_time_available > 0)
        ? (static_cast<double>(total_cpu_work) / total_cpu_time_available * 100.0)
        : 0.0;

    int64_t idle_time = total_cpu_time_available - total_cpu_work;

    logger::info("======================================================================");
    logger::info("Simulation completed at t={}", simulation_time);
    logger::info("======================================================================");

    if (verbose) {
        logger::info("");
        logger::info("Host Statistics:");
        logger::info("----------------------------------------------------------------------");

        for (const auto& host : hosts_) {
            int64_t host_cpu_work = cpu_work_per_host[host->name];
            int64_t host_cpu_available = host->cpu_cores * simulation_time;
            double host_utilization = (host_cpu_available > 0)
                ? (static_cast<double>(host_cpu_work) / host_cpu_available * 100.0)
                : 0.0;

            logger::info("{} ({} cores):", host->name, host->cpu_cores);
            logger::info("  CPU work time:      {}", host_cpu_work);
            logger::info("  CPU available time: {} ({} cores × {})",
                        host_cpu_available, host->cpu_cores, simulation_time);
            logger::info("  CPU idle time:      {}", host_cpu_available - host_cpu_work);
            logger::info("  CPU utilization:    {:.2f}%", host_utilization);
        }

        logger::info("----------------------------------------------------------------------");
    }

    logger::info("");
    logger::info("Overall Statistics:");
    logger::info("----------------------------------------------------------------------");
    logger::info("Total CPU cores:        {}", total_cpu_cores);
    logger::info("Total CPU work time:    {}", total_cpu_work);

    // Detailed breakdown of CPU available time
    if (verbose) {
        logger::info("Total CPU available:    {}", total_cpu_time_available);
        logger::info("  Breakdown:");
        for (const auto& host : hosts_) {
            int64_t host_cpu_available = host->cpu_cores * simulation_time;
            logger::info("    {}: {} cores × {} = {}",
                        host->name, host->cpu_cores, simulation_time, host_cpu_available);
        }
    } else {
        logger::info("Total CPU available:    {} ({} cores × {})",
                    total_cpu_time_available, total_cpu_cores, simulation_time);
    }

    logger::info("Total CPU idle time:    {}", idle_time);
    logger::info("CPU utilization:        {:.2f}%", cpu_utilization);
    logger::info("======================================================================");
}

} // namespace simulator
