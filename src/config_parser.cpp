#include "../include/config_parser.h"
#include <tinyxml2.h>
#include <stdexcept>
#include <iostream>
#include <filesystem>

namespace parsers {

std::unordered_map<std::string, models::ExperimentConfig>
load_experiments_from_xml(const std::string& xml_path) {
    // Check if file exists
    if (!std::filesystem::exists(xml_path)) {
        throw std::runtime_error("Configuration file not found: " + xml_path);
    }

    // Get the directory containing the XML file for resolving relative paths
    std::filesystem::path xml_dir = std::filesystem::path(xml_path).parent_path();
    if (xml_dir.empty()) {
        xml_dir = ".";
        
    }
   
    tinyxml2::XMLDocument doc;
    if (doc.LoadFile(xml_path.c_str()) != tinyxml2::XML_SUCCESS) {
        throw std::runtime_error("Failed to load XML file: " + xml_path);
    }

    auto* root = doc.FirstChildElement("experiments");
    if (!root) {
        throw std::runtime_error("Root element 'experiments' not found");
    }

    std::unordered_map<std::string, models::ExperimentConfig> configs;

    // Iterate through experiment elements
    for (auto* experiment = root->FirstChildElement("experiment");
         experiment != nullptr;
         experiment = experiment->NextSiblingElement("experiment")) {

        const char* name = experiment->Attribute("name");
        if (!name) {
            throw std::runtime_error("Experiment missing 'name' attribute");
        }

        models::ExperimentConfig config;

        // Parse tasks CSV path
        auto* tasks_elem = experiment->FirstChildElement("tasks");
        if (!tasks_elem || !tasks_elem->GetText()) {
            throw std::runtime_error("Experiment '" + std::string(name) +
                                   "' missing 'tasks' element");
        }
        config.tasks_csv_path = tasks_elem->GetText();

        std::filesystem::path csv_path = tasks_elem->GetText();
        if (csv_path.is_relative()) {
            csv_path = xml_dir / csv_path;
            
        }
        config.tasks_csv_path = csv_path.lexically_normal().string();
             
        // Iterate through host elements
        for (auto* host = experiment->FirstChildElement("host");
             host != nullptr;
             host = host->NextSiblingElement("host")) {

            const char* host_id = host->Attribute("id");
            if (!host_id) {
                throw std::runtime_error("Host missing 'id' attribute in experiment '" +
                                       std::string(name) + "'");
            }

            auto* cpu_cores_elem = host->FirstChildElement("cpu_cores");
            auto* ram_elem = host->FirstChildElement("ram");

            if (!cpu_cores_elem || !ram_elem) {
                throw std::runtime_error("Missing cpu_cores or ram for " + std::string(host_id));
            }

            int cpu_cores = 0;
            int ram = 0;

            if (cpu_cores_elem->QueryIntText(&cpu_cores) != tinyxml2::XML_SUCCESS) {
                throw std::runtime_error("Invalid cpu_cores value for " + std::string(host_id));
            }

            if (ram_elem->QueryIntText(&ram) != tinyxml2::XML_SUCCESS) {
                throw std::runtime_error("Invalid ram value for " + std::string(host_id));
            }

            models::HostConfig host_config{cpu_cores, ram};
            host_config.validate();

            config.hosts[host_id] = host_config;
        }

        if (config.hosts.empty()) {
            throw std::runtime_error("Experiment '" + std::string(name) +
                                   "' must have at least 1 host");
        }

        config.validate(false);
        configs[name] = config;
    }

    return configs;
}

models::ExperimentConfig get_experiment_config(
    const std::unordered_map<std::string, models::ExperimentConfig>& configs,
    const std::string& config_name) {

    auto it = configs.find(config_name);
    if (it == configs.end()) {
        std::string available;
        for (const auto& [name, _] : configs) {
            if (!available.empty()) available += ", ";
            available += name;
        }
        throw std::invalid_argument("Unknown host configuration: '" + config_name + "'. " +
                                  "Available configurations: " + available);
    }

    return it->second;
}

} // namespace parsers
