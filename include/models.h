// Data models for the simulation

#ifndef MODELS_H_
#define MODELS_H_

#include <string>
#include <vector>
#include <unordered_map>
#include <stdexcept>

namespace models {

// Represents a task to be executed on a host
struct Task {
    std::string name;
    std::string host;
    int initial_sleep_time;
    int run_time;
    int ram;
    int network_time;
    std::vector<std::string> dependencies;
    std::vector<size_t> dependency_indices;
    size_t index;
    size_t host_index;

    // Check if task has dependencies
    bool has_dependency() const {
        return !dependency_indices.empty();
    }


    // Validate task parameters
    void validate() const {
        if (initial_sleep_time < 0) {
            throw std::invalid_argument("Initial sleep time must be >= 0, got " + std::to_string(initial_sleep_time));
        }
        if (run_time < 0) {
            throw std::invalid_argument("Run time must be >= 0, got " + std::to_string(run_time));
        }
        if (ram < 0) {
            throw std::invalid_argument("RAM must be >= 0, got " + std::to_string(ram));
        }
        if (network_time < 0) {
            throw std::invalid_argument("Network time must be >= 0, got " + std::to_string(network_time));
        }
    }
};

// Configuration for a single host
struct HostConfig {
    int cpu_cores;
    int ram;

    void validate() const {
        if (cpu_cores <= 0) {
            throw std::invalid_argument("CPU cores must be > 0, got " + std::to_string(cpu_cores));
        }
        if (ram <= 0) {
            throw std::invalid_argument("RAM must be > 0, got " + std::to_string(ram));
        }
    }
};

// Configuration for an experiment containing multiple hosts and tasks
struct ExperimentConfig {
    std::unordered_map<std::string, HostConfig> hosts;
    std::string tasks_csv_path;  // Path to CSV file with tasks

    void validate(bool validate_hosts=false) const {
        if (hosts.empty()) {
            throw std::invalid_argument("Experiment configuration must have at least one host");
        }
        if (tasks_csv_path.empty()) {
            throw std::invalid_argument("Experiment configuration must specify tasks CSV path");
        }
        if (validate_hosts) {
            for (const auto& [host_id, host_config] : hosts) {
                host_config.validate();
            }
        }
    }
};

} // namespace models

#endif // MODELS_H_
