// XML configuration parser

#ifndef CONFIG_PARSER_H_
#define CONFIG_PARSER_H_

#include "models.h"
#include <string>
#include <unordered_map>

namespace parsers {

// Load experiment configurations from XML file
std::unordered_map<std::string, models::ExperimentConfig>
load_experiments_from_xml(const std::string& xml_path);

// Get a specific experiment configuration by name
models::ExperimentConfig get_experiment_config(
    const std::unordered_map<std::string, models::ExperimentConfig>& configs,
    const std::string& config_name);

} // namespace parsers

#endif // CONFIG_PARSER_H_
