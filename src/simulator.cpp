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
NetworkLink::NetworkLink(simcpp20::simulation<>& sim, const std::vector<std::string>& host_ids) {
    // Create full-duplex links between all pairs of hosts
    for (const auto& from_host : host_ids) {
        for (const auto& to_host : host_ids) {
            if (from_host != to_host) {
                // Create a unique network link for this direction
                auto key = std::make_pair(from_host, to_host);
                links_[key] = std::make_unique<simcpp20::resource<>>(sim, 1);
            }
        }
    }

    logger::info("Network initialized with {} directional links for {} hosts", links_.size(), host_ids.size());
}

simcpp20::resource<>* NetworkLink::get_link(const std::string& from_host,
                                            const std::string& to_host) {
    auto key = std::make_pair(from_host, to_host);
    auto it = links_.find(key);

    if (it == links_.end()) {
        throw std::runtime_error("No network link from " + from_host + " to " + to_host);
    }

    return it->second.get();
}

// Task process coroutine
simcpp20::process<> task_process(
    simcpp20::simulation<>& sim,
    const models::Task& task,
    size_t task_index,
    const std::unordered_map<std::string, HostPtr>& hosts,
    NetworkLinkPtr network,
    std::vector<simcpp20::event<>>& task_completed,
    const std::unordered_map<std::string, size_t>& task_name_to_index,
    const std::vector<models::Task>& tasks) {

    // Step 1: Initial sleep
    if (task.initial_sleep_time > 0) {
        logger::debug("[{}]\t[t={}]\tTask {}: Sleeping for {} time units",
                     task.host, static_cast<int>(sim.now()), task.name, task.initial_sleep_time);
        co_await sim.timeout(task.initial_sleep_time);
    }

    // Step 2: Wait for dependency if exists
    if (task.has_dependency()) {
        logger::debug("[{}]\t[t={}]\tTask {}: Waiting for dependency {}",
                     task.host, static_cast<int>(sim.now()), task.name, *task.dependency);

        // Find dependency index
        size_t dep_index = task_name_to_index.at(*task.dependency);

        // Wait for dependency task to complete, O(1) access
        co_await task_completed[dep_index];

        // Get dependency task for cross-host check
        const auto& dep_task = tasks[dep_index];

        // If cross-host dependency, wait for network transmission
        if (dep_task.host != task.host) {
            if (dep_task.network_time > 0) {
                auto* link = network->get_link(dep_task.host, task.host);

                logger::debug("[{}]\t[t={}]\tTask {}: Waiting for network transmission from {} ({} time units)",
                             task.host, static_cast<int>(sim.now()), task.name,
                             *task.dependency, dep_task.network_time);

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
    auto host = hosts.at(task.host);

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

    // Build task name to index mapping (copy names to index table for fast access)
    for (size_t i = 0; i < tasks_.size(); ++i) {
        task_name_to_index_[tasks_[i].name] = i;
    }

    // Create hosts
    std::vector<std::string> host_ids;
    for (const auto& [host_id, host_config] : config.hosts) {
        hosts_[host_id] = std::make_shared<Host>(
            sim_, host_id, host_config.cpu_cores, host_config.ram);
        host_ids.push_back(host_id);
    }

    // Create network link with all host IDs
    network_ = std::make_shared<NetworkLink>(sim_, host_ids);

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
    int total_cpu_work = 0;
    std::unordered_map<std::string, int> cpu_work_per_host;

    for (const auto& task : tasks_) {
        total_cpu_work += task.run_time;
        cpu_work_per_host[task.host] += task.run_time;
    }

    // Schedule all tasks (start their coroutines)
    for (size_t i = 0; i < tasks_.size(); ++i) {
        task_process(sim_, tasks_[i], i, hosts_, network_, task_completed_, task_name_to_index_, tasks_);
    }

    // Run simulation
    sim_.run();

    // Calculate metrics
    int simulation_time = static_cast<int>(sim_.now());

    // Calculate total available CPU time across all hosts
    int total_cpu_cores = 0;
    for (const auto& [host_id, host] : hosts_) {
        total_cpu_cores += host->cpu_cores;
    }
    int total_cpu_time_available = total_cpu_cores * simulation_time;

    // Calculate CPU utilization
    double cpu_utilization = (total_cpu_time_available > 0)
        ? (static_cast<double>(total_cpu_work) / total_cpu_time_available * 100.0)
        : 0.0;

    int idle_time = total_cpu_time_available - total_cpu_work;

    logger::info("======================================================================");
    logger::info("Simulation completed at t={}", simulation_time);
    logger::info("======================================================================");

    if (verbose) {
        logger::info("");
        logger::info("Host Statistics:");
        logger::info("----------------------------------------------------------------------");

        for (const auto& [host_id, host] : hosts_) {
            int host_cpu_work = cpu_work_per_host[host_id];
            int host_cpu_available = host->cpu_cores * simulation_time;
            double host_utilization = (host_cpu_available > 0)
                ? (static_cast<double>(host_cpu_work) / host_cpu_available * 100.0)
                : 0.0;

            logger::info("{} ({} cores):", host_id, host->cpu_cores);
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
        for (const auto& [host_id, host] : hosts_) {
            int host_cpu_available = host->cpu_cores * simulation_time;
            logger::info("    {}: {} cores × {} = {}",
                        host_id, host->cpu_cores, simulation_time, host_cpu_available);
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
