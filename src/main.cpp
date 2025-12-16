// Main application for running task simulations using SimCpp20

#include "simulator.hpp"
#include "config_parser.h"
#include "csv_parser.h"
#include "logger.hpp"
#include <iostream>
#include <string>
#include <cstring>
#include <filesystem>
#include <sstream>

void print_usage(const char* program_name) {
    std::cout << "Task Simulator - Simulates task execution on multi-host system\n\n";
    std::cout << "Usage: " << program_name << " <experiments_xml> --experiment <name> [options]\n\n";
    std::cout << "Arguments:\n";
    std::cout << "  experiments_xml           Path to XML file containing experiment definitions\n";
    std::cout << "  --experiment, -e NAME     Experiment name to run\n\n";
    std::cout << "Options:\n";
    std::cout << "  --help, -h                Show this help message\n";
    std::cout << "  --verbose, -v             Show detailed statistics\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << program_name << " experiments.xml --experiment simple\n";
    std::cout << "  " << program_name << " experiments.xml -e ping_pong --verbose\n";
}

struct Args {
    std::string xml_file;
    std::string experiment_name;
    bool show_help = false;
    bool verbose = false;
};

Args parse_arguments(int argc, char* argv[]) {
    Args args;

    if (argc < 2) {
        return args;
    }

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            args.show_help = true;
            return args;
        } else if (arg == "--verbose" || arg == "-v") {
            args.verbose = true;
        } else if (arg == "--experiment" || arg == "-e") {
            if (i + 1 < argc) {
                args.experiment_name = argv[++i];
            } else {
                throw std::invalid_argument("--experiment requires an argument");
            }
        } else if (arg[0] != '-') {
            // Positional argument (XML file)
            if (args.xml_file.empty()) {
                args.xml_file = arg;
            } else {
                throw std::invalid_argument("Multiple XML files specified");
            }
        } else {
            throw std::invalid_argument("Unknown option: " + arg);
        }
    }

    return args;
}

int main(int argc, char* argv[]) {
    // Initialize logger
    logger::init();

    try {
        auto args = parse_arguments(argc, argv);

        if (args.show_help || args.xml_file.empty() || args.experiment_name.empty()) {
            print_usage(argv[0]);
            if (!args.show_help && (args.xml_file.empty() || args.experiment_name.empty())) {
                std::cerr << "\nError: experiments_xml and --experiment are required arguments\n";
                return 1;
            }
            return 0;
        }

        // Step 1: Load experiment configuration from XML
        logger::info("Loading experiments from: {}", args.xml_file);
        auto experiments = parsers::load_experiments_from_xml(args.xml_file);

        logger::info("Loading experiment: {}", args.experiment_name);
        auto experiment = parsers::get_experiment_config(experiments, args.experiment_name);

        // Log experiment configuration
        logger::info("Experiment configuration:");
        logger::info("  Tasks CSV: {}", experiment.tasks_csv_path);
        std::ostringstream hosts_info;
        bool first = true;
        for (const auto& [host_id, host_cfg] : experiment.hosts) {
            if (!first) hosts_info << "; ";
            hosts_info << host_id << " (" << host_cfg.cpu_cores << " cores, "
                      << host_cfg.ram << " RAM)";
            first = false;
        }
        logger::info("  Hosts: {}", hosts_info.str());

        // Step 2: Parse tasks from CSV (path from experiment config)
        logger::info("Parsing tasks from CSV: {}", experiment.tasks_csv_path);
        auto tasks = parsers::parse_tasks_csv(experiment.tasks_csv_path);
        logger::info("Parsed {} tasks", tasks.size());

        // Step 3: Validate task dependencies
        logger::info("Validating task dependencies...");
        parsers::validate_task_dependencies(tasks);
        logger::info("Dependencies validated successfully");

        // Step 4: Run simulation
        logger::info("Initializing simulator...");
        simulator::TaskSimulator sim(experiment, std::move(tasks));

        logger::info("Starting simulation...");
        sim.run(args.verbose);

        logger::info("Simulation completed successfully!");

        return 0;

    } catch (const std::exception& e) {
        logger::error("{}", e.what());
        return 1;
    }
}
