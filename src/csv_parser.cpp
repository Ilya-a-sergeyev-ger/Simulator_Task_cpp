#include "../include/csv_parser.h"
#include <fstream>
#include <sstream>
#include <unordered_set>
#include <unordered_map>
#include <stdexcept>
#include <filesystem>
#include <algorithm>

namespace parsers {

// Helper function to trim whitespace
static std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, last - first + 1);
}

// Helper function to parse CSV line
static std::vector<std::string> parse_csv_line(const std::string& line) {
    std::vector<std::string> fields;
    std::string field;
    std::istringstream line_stream(line);

    while (std::getline(line_stream, field, ',')) {
        fields.push_back(trim(field));
    }

    // Handle trailing comma (empty last field)
    if (!line.empty() && line.back() == ',') {
        fields.push_back("");
    }

    return fields;
}

std::vector<models::Task> parse_tasks_csv(const std::string& csv_path) {
    if (!std::filesystem::exists(csv_path)) {
        throw std::runtime_error("Task CSV file not found: " + csv_path);
    }

    std::ifstream file(csv_path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open CSV file: " + csv_path);
    }

    std::vector<models::Task> tasks;
    std::string line;
    int row_num = 1;

    // Read header
    if (!std::getline(file, line)) {
        throw std::runtime_error("CSV file is empty or has no header");
    }

    auto headers = parse_csv_line(line);

    // Validate header
    std::unordered_set<std::string> expected_headers = {
        "TASK_NAME", "TASK_HOST", "TASK_INITIAL_SLEEP_TIME",
        "TASK_RUN_TIME", "TASK_RAM", "TASK_NETWORK_TIME", "TASK_DEPENDENCY"
    };

    std::unordered_set<std::string> actual_headers(headers.begin(), headers.end());

    if (actual_headers != expected_headers) {
        std::string error = "Invalid CSV header. ";

        // Check missing columns
        std::vector<std::string> missing;
        for (const auto& h : expected_headers) {
            if (actual_headers.find(h) == actual_headers.end()) {
                missing.push_back(h);
            }
        }
        if (!missing.empty()) {
            error += "Missing columns: ";
            for (size_t i = 0; i < missing.size(); i++) {
                if (i > 0) error += ", ";
                error += missing[i];
            }
        }

        // Check extra columns
        std::vector<std::string> extra;
        for (const auto& h : actual_headers) {
            if (expected_headers.find(h) == expected_headers.end()) {
                extra.push_back(h);
            }
        }
        if (!extra.empty()) {
            if (!missing.empty()) error += ". ";
            error += "Extra columns: ";
            for (size_t i = 0; i < extra.size(); i++) {
                if (i > 0) error += ", ";
                error += extra[i];
            }
        }

        throw std::runtime_error(error);
    }

    // Create header index map
    std::unordered_map<std::string, size_t> header_index;
    for (size_t i = 0; i < headers.size(); i++) {
        header_index[headers[i]] = i;
    }

    // Parse rows
    while (std::getline(file, line)) {
        row_num++;
        auto fields = parse_csv_line(line);

        if (fields.size() != headers.size()) {
            throw std::runtime_error("Row " + std::to_string(row_num) +
                                   ": Expected " + std::to_string(headers.size()) +
                                   " fields, got " + std::to_string(fields.size()));
        }

        try {
            std::string task_name = fields[header_index["TASK_NAME"]];
            if (task_name.empty()) {
                throw std::runtime_error("TASK_NAME cannot be empty");
            }

            std::string task_host = fields[header_index["TASK_HOST"]];
            int initial_sleep_time = std::stoi(fields[header_index["TASK_INITIAL_SLEEP_TIME"]]);
            int run_time = std::stoi(fields[header_index["TASK_RUN_TIME"]]);
            int ram = std::stoi(fields[header_index["TASK_RAM"]]);
            int network_time = std::stoi(fields[header_index["TASK_NETWORK_TIME"]]);
            std::string dependency_str = fields[header_index["TASK_DEPENDENCY"]];

            std::optional<std::string> dependency;
            if (!dependency_str.empty()) {
                dependency = dependency_str;
            }

            models::Task task{
                task_name,
                task_host,
                initial_sleep_time,
                run_time,
                ram,
                network_time,
                dependency
            };

            task.validate();
            tasks.push_back(task);

        } catch (const std::exception& e) {
            throw std::runtime_error("Error parsing row " + std::to_string(row_num) + ": " + e.what());
        }
    }

    return tasks;
}

// Helper function for cycle detection (DFS)
static bool has_cycle(const std::string& task_name,
                      const std::unordered_map<std::string, models::Task>& task_dict,
                      std::unordered_set<std::string>& visited,
                      std::unordered_set<std::string>& rec_stack) {
    visited.insert(task_name);
    rec_stack.insert(task_name);

    const auto& task = task_dict.at(task_name);
    if (task.has_dependency()) {
        const auto& dep = *task.dependency;

        if (visited.find(dep) == visited.end()) {
            if (has_cycle(dep, task_dict, visited, rec_stack)) {
                return true;
            }
        } else if (rec_stack.find(dep) != rec_stack.end()) {
            return true;
        }
    }

    rec_stack.erase(task_name);
    return false;
}

void validate_task_dependencies(const std::vector<models::Task>& tasks) {
    // Build task name set and task dict
    std::unordered_set<std::string> task_names;
    std::unordered_map<std::string, models::Task> task_dict;

    for (const auto& task : tasks) {
        task_names.insert(task.name);
        task_dict[task.name] = task;
    }

    // Check that all dependencies exist
    for (const auto& task : tasks) {
        if (task.has_dependency()) {
            if (task_names.find(*task.dependency) == task_names.end()) {
                throw std::runtime_error("Task '" + task.name +
                                       "' has undefined dependency: '" + *task.dependency + "'");
            }
        }
    }

    // Check for circular dependencies using DFS
    std::unordered_set<std::string> visited;

    for (const auto& task : tasks) {
        if (visited.find(task.name) == visited.end()) {
            std::unordered_set<std::string> rec_stack;
            if (has_cycle(task.name, task_dict, visited, rec_stack)) {
                throw std::runtime_error("Circular dependency detected involving task '" +
                                       task.name + "'");
            }
        }
    }
}

} // namespace parsers
