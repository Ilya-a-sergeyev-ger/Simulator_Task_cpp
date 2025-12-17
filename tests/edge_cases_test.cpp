#include <gtest/gtest.h>
#include "../include/config_parser.h"
#include "../include/csv_parser.h"
#include "../include/simulator.hpp"
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

// Test fixture for edge cases
class EdgeCaseTest : public ::testing::Test {
protected:
    std::string test_dir;

    void SetUp() override {
        test_dir = "/tmp/run_simulation_tests";
        fs::create_directories(test_dir);
    }

    void TearDown() override {
        fs::remove_all(test_dir);
    }

    void write_file(const std::string& filename, const std::string& content) {
        std::ofstream file(test_dir + "/" + filename);
        file << content;
        file.close();
    }
};

// ============================================================================
// XML Parsing Error Tests
// ============================================================================

TEST_F(EdgeCaseTest, MissingXMLFile) {
    EXPECT_THROW(
        parsers::load_experiments_from_xml(test_dir + "/nonexistent.xml"),
        std::runtime_error
    );
}

TEST_F(EdgeCaseTest, InvalidXMLSyntax) {
    write_file("invalid.xml", "<?xml version=\"1.0\"?><experiments><broken");

    EXPECT_THROW(
        parsers::load_experiments_from_xml(test_dir + "/invalid.xml"),
        std::runtime_error
    );
}

TEST_F(EdgeCaseTest, MissingExperimentsRoot) {
    write_file("no_root.xml",
        "<?xml version=\"1.0\"?>\n"
        "<wrong_root>\n"
        "</wrong_root>\n"
    );

    EXPECT_THROW(
        parsers::load_experiments_from_xml(test_dir + "/no_root.xml"),
        std::runtime_error
    );
}

TEST_F(EdgeCaseTest, ExperimentWithoutName) {
    write_file("no_name.xml",
        "<?xml version=\"1.0\"?>\n"
        "<experiments>\n"
        "  <experiment>\n"
        "    <tasks>tasks.csv</tasks>\n"
        "    <host id=\"HOST_0\"><cpu_cores>1</cpu_cores><ram>1000</ram></host>\n"
        "  </experiment>\n"
        "</experiments>\n"
    );

    EXPECT_THROW(
        parsers::load_experiments_from_xml(test_dir + "/no_name.xml"),
        std::runtime_error
    );
}

TEST_F(EdgeCaseTest, ExperimentWithoutTasks) {
    write_file("no_tasks.xml",
        "<?xml version=\"1.0\"?>\n"
        "<experiments>\n"
        "  <experiment name=\"test\">\n"
        "    <host id=\"HOST_0\"><cpu_cores>1</cpu_cores><ram>1000</ram></host>\n"
        "  </experiment>\n"
        "</experiments>\n"
    );

    EXPECT_THROW(
        parsers::load_experiments_from_xml(test_dir + "/no_tasks.xml"),
        std::runtime_error
    );
}

TEST_F(EdgeCaseTest, HostWithoutID) {
    write_file("no_host_id.xml",
        "<?xml version=\"1.0\"?>\n"
        "<experiments>\n"
        "  <experiment name=\"test\">\n"
        "    <tasks>tasks.csv</tasks>\n"
        "    <host><cpu_cores>1</cpu_cores><ram>1000</ram></host>\n"
        "  </experiment>\n"
        "</experiments>\n"
    );

    EXPECT_THROW(
        parsers::load_experiments_from_xml(test_dir + "/no_host_id.xml"),
        std::runtime_error
    );
}

TEST_F(EdgeCaseTest, HostWithoutCPUCores) {
    write_file("no_cpu.xml",
        "<?xml version=\"1.0\"?>\n"
        "<experiments>\n"
        "  <experiment name=\"test\">\n"
        "    <tasks>tasks.csv</tasks>\n"
        "    <host id=\"HOST_0\"><ram>1000</ram></host>\n"
        "  </experiment>\n"
        "</experiments>\n"
    );

    EXPECT_THROW(
        parsers::load_experiments_from_xml(test_dir + "/no_cpu.xml"),
        std::runtime_error
    );
}

TEST_F(EdgeCaseTest, ZeroCPUCores) {
    write_file("zero_cpu.xml",
        "<?xml version=\"1.0\"?>\n"
        "<experiments>\n"
        "  <experiment name=\"test\">\n"
        "    <tasks>tasks.csv</tasks>\n"
        "    <host id=\"HOST_0\"><cpu_cores>0</cpu_cores><ram>1000</ram></host>\n"
        "  </experiment>\n"
        "</experiments>\n"
    );

    EXPECT_THROW(
        parsers::load_experiments_from_xml(test_dir + "/zero_cpu.xml"),
        std::invalid_argument
    );
}

TEST_F(EdgeCaseTest, NegativeRAM) {
    write_file("negative_ram.xml",
        "<?xml version=\"1.0\"?>\n"
        "<experiments>\n"
        "  <experiment name=\"test\">\n"
        "    <tasks>tasks.csv</tasks>\n"
        "    <host id=\"HOST_0\"><cpu_cores>1</cpu_cores><ram>-500</ram></host>\n"
        "  </experiment>\n"
        "</experiments>\n"
    );

    EXPECT_THROW(
        parsers::load_experiments_from_xml(test_dir + "/negative_ram.xml"),
        std::invalid_argument
    );
}

TEST_F(EdgeCaseTest, ExperimentWithoutHosts) {
    write_file("no_hosts.xml",
        "<?xml version=\"1.0\"?>\n"
        "<experiments>\n"
        "  <experiment name=\"test\">\n"
        "    <tasks>tasks.csv</tasks>\n"
        "  </experiment>\n"
        "</experiments>\n"
    );

    EXPECT_THROW(
        parsers::load_experiments_from_xml(test_dir + "/no_hosts.xml"),
        std::runtime_error
    );
}

// ============================================================================
// CSV Parsing Error Tests
// ============================================================================

TEST_F(EdgeCaseTest, MissingCSVFile) {
    EXPECT_THROW(
        parsers::parse_tasks_csv(test_dir + "/nonexistent.csv"),
        std::runtime_error
    );
}

TEST_F(EdgeCaseTest, EmptyCSV) {
    write_file("empty.csv", "");

    EXPECT_THROW(
        parsers::parse_tasks_csv(test_dir + "/empty.csv"),
        std::runtime_error
    );
}

TEST_F(EdgeCaseTest, CSVWrongFieldCount) {
    write_file("wrong_fields.csv",
        "name,host,initial_sleep_time,run_time,ram,network_time,dependency\n"
        "Task1,HOST_0,0,5,100\n"
    );

    EXPECT_THROW(
        parsers::parse_tasks_csv(test_dir + "/wrong_fields.csv"),
        std::runtime_error
    );
}

TEST_F(EdgeCaseTest, CSVNegativeRunTime) {
    write_file("negative_runtime.csv",
        "TASK_NAME,TASK_HOST,TASK_INITIAL_SLEEP_TIME,TASK_RUN_TIME,TASK_RAM,TASK_NETWORK_TIME,TASK_DEPENDENCY\n"
        "Task1,HOST_0,0,-5,100,0,\n"
    );

    EXPECT_THROW(
        parsers::parse_tasks_csv(test_dir + "/negative_runtime.csv"),
        std::runtime_error
    );
}

TEST_F(EdgeCaseTest, CSVNegativeRAM) {
    write_file("negative_ram_csv.csv",
        "TASK_NAME,TASK_HOST,TASK_INITIAL_SLEEP_TIME,TASK_RUN_TIME,TASK_RAM,TASK_NETWORK_TIME,TASK_DEPENDENCY\n"
        "Task1,HOST_0,0,5,-100,0,\n"
    );

    EXPECT_THROW(
        parsers::parse_tasks_csv(test_dir + "/negative_ram_csv.csv"),
        std::runtime_error
    );
}

// ============================================================================
// Dependency Validation Tests
// ============================================================================

TEST_F(EdgeCaseTest, CircularDependency) {
    write_file("circular.csv",
        "TASK_NAME,TASK_HOST,TASK_INITIAL_SLEEP_TIME,TASK_RUN_TIME,TASK_RAM,TASK_NETWORK_TIME,TASK_DEPENDENCY\n"
        "TaskA,HOST_0,0,5,100,0,TaskC\n"
        "TaskB,HOST_0,0,5,100,0,TaskA\n"
        "TaskC,HOST_0,0,5,100,0,TaskB\n"
    );

    auto tasks = parsers::parse_tasks_csv(test_dir + "/circular.csv");

    EXPECT_THROW(
        parsers::validate_task_dependencies(tasks),
        std::runtime_error
    );
}

TEST_F(EdgeCaseTest, SelfDependency) {
    write_file("self_dep.csv",
        "TASK_NAME,TASK_HOST,TASK_INITIAL_SLEEP_TIME,TASK_RUN_TIME,TASK_RAM,TASK_NETWORK_TIME,TASK_DEPENDENCY\n"
        "Task1,HOST_0,0,5,100,0,Task1\n"
    );

    auto tasks = parsers::parse_tasks_csv(test_dir + "/self_dep.csv");

    EXPECT_THROW(
        parsers::validate_task_dependencies(tasks),
        std::runtime_error
    );
}

TEST_F(EdgeCaseTest, MissingDependency) {
    write_file("missing_dep.csv",
        "TASK_NAME,TASK_HOST,TASK_INITIAL_SLEEP_TIME,TASK_RUN_TIME,TASK_RAM,TASK_NETWORK_TIME,TASK_DEPENDENCY\n"
        "Task1,HOST_0,0,5,100,0,NonExistent\n"
    );

    auto tasks = parsers::parse_tasks_csv(test_dir + "/missing_dep.csv");

    EXPECT_THROW(
        parsers::validate_task_dependencies(tasks),
        std::runtime_error
    );
}

TEST_F(EdgeCaseTest, LongDependencyChain) {
    std::stringstream csv;
    csv << "TASK_NAME,TASK_HOST,TASK_INITIAL_SLEEP_TIME,TASK_RUN_TIME,TASK_RAM,TASK_NETWORK_TIME,TASK_DEPENDENCY\n";
    csv << "T0,HOST_0,0,1,100,0,\n";

    // Create chain of 100 tasks
    for (int i = 1; i < 100; i++) {
        csv << "T" << i << ",HOST_0,0,1,100,0,T" << (i-1) << "\n";
    }

    write_file("long_chain.csv", csv.str());

    auto tasks = parsers::parse_tasks_csv(test_dir + "/long_chain.csv");

    // Should validate successfully (no cycles)
    EXPECT_NO_THROW(parsers::validate_task_dependencies(tasks));
    EXPECT_EQ(tasks.size(), 100);
}

// ============================================================================
// Simulation Runtime Tests
// ============================================================================

TEST_F(EdgeCaseTest, TaskWaitsForRAMToBeReleased) {
    write_file("config.xml",
        "<?xml version=\"1.0\"?>\n"
        "<experiments>\n"
        "  <experiment name=\"test\">\n"
        "    <tasks>" + test_dir + "/tasks.csv</tasks>\n"
        "    <host id=\"HOST_0\"><cpu_cores>2</cpu_cores><ram>1000</ram></host>\n"
        "  </experiment>\n"
        "</experiments>\n"
    );

    write_file("tasks.csv",
        "TASK_NAME,TASK_HOST,TASK_INITIAL_SLEEP_TIME,TASK_RUN_TIME,TASK_RAM,TASK_NETWORK_TIME,TASK_DEPENDENCY\n"
        "Task1,HOST_0,0,10,800,0,\n"
        "Task2,HOST_0,0,5,800,0,\n"
    );

    auto experiments = parsers::load_experiments_from_xml(test_dir + "/config.xml");
    auto experiment = parsers::get_experiment_config(experiments, "test");
    auto tasks = parsers::parse_tasks_csv(test_dir + "/tasks.csv");

    simulator::TaskSimulator sim(experiment, std::move(tasks));

    // Both tasks need 800 RAM but host only has 1000 total
    // Task2 should wait for Task1 to release RAM before it can start
    // Simulation should complete successfully
    EXPECT_NO_THROW(sim.run());
}

TEST_F(EdgeCaseTest, TaskReferencesUnknownHost) {
    write_file("config.xml",
        "<?xml version=\"1.0\"?>\n"
        "<experiments>\n"
        "  <experiment name=\"test\">\n"
        "    <tasks>" + test_dir + "/tasks.csv</tasks>\n"
        "    <host id=\"HOST_0\"><cpu_cores>1</cpu_cores><ram>1000</ram></host>\n"
        "  </experiment>\n"
        "</experiments>\n"
    );

    write_file("tasks.csv",
        "TASK_NAME,TASK_HOST,TASK_INITIAL_SLEEP_TIME,TASK_RUN_TIME,TASK_RAM,TASK_NETWORK_TIME,TASK_DEPENDENCY\n"
        "Task1,HOST_999,0,10,100,0,\n"
    );

    auto experiments = parsers::load_experiments_from_xml(test_dir + "/config.xml");
    auto experiment = parsers::get_experiment_config(experiments, "test");
    auto tasks = parsers::parse_tasks_csv(test_dir + "/tasks.csv");

    EXPECT_THROW(
        simulator::TaskSimulator sim(experiment, std::move(tasks)),
        std::runtime_error
    );
}

TEST_F(EdgeCaseTest, UnknownExperimentName) {
    write_file("config.xml",
        "<?xml version=\"1.0\"?>\n"
        "<experiments>\n"
        "  <experiment name=\"test\">\n"
        "    <tasks>tasks.csv</tasks>\n"
        "    <host id=\"HOST_0\"><cpu_cores>1</cpu_cores><ram>1000</ram></host>\n"
        "  </experiment>\n"
        "</experiments>\n"
    );

    auto experiments = parsers::load_experiments_from_xml(test_dir + "/config.xml");

    EXPECT_THROW(
        parsers::get_experiment_config(experiments, "nonexistent"),
        std::invalid_argument
    );
}

// ============================================================================
// Success Cases (Valid Edge Cases)
// ============================================================================

TEST_F(EdgeCaseTest, ValidLongDependencyChain) {
    write_file("config.xml",
        "<?xml version=\"1.0\"?>\n"
        "<experiments>\n"
        "  <experiment name=\"test\">\n"
        "    <tasks>" + test_dir + "/tasks.csv</tasks>\n"
        "    <host id=\"HOST_0\"><cpu_cores>1</cpu_cores><ram>5000</ram></host>\n"
        "  </experiment>\n"
        "</experiments>\n"
    );

    std::stringstream csv;
    csv << "TASK_NAME,TASK_HOST,TASK_INITIAL_SLEEP_TIME,TASK_RUN_TIME,TASK_RAM,TASK_NETWORK_TIME,TASK_DEPENDENCY\n";
    csv << "T0,HOST_0,0,1,100,0,\n";

    // Create chain of 50 tasks
    for (int i = 1; i < 50; i++) {
        csv << "T" << i << ",HOST_0,0,1,100,0,T" << (i-1) << "\n";
    }

    write_file("tasks.csv", csv.str());

    auto experiments = parsers::load_experiments_from_xml(test_dir + "/config.xml");
    auto experiment = parsers::get_experiment_config(experiments, "test");
    auto tasks = parsers::parse_tasks_csv(test_dir + "/tasks.csv");

    parsers::validate_task_dependencies(tasks);

    simulator::TaskSimulator sim(experiment, std::move(tasks));

    // Should complete successfully
    EXPECT_NO_THROW(sim.run());
}

TEST_F(EdgeCaseTest, TaskWithZeroResources) {
    write_file("config.xml",
        "<?xml version=\"1.0\"?>\n"
        "<experiments>\n"
        "  <experiment name=\"test\">\n"
        "    <tasks>" + test_dir + "/tasks.csv</tasks>\n"
        "    <host id=\"HOST_0\"><cpu_cores>1</cpu_cores><ram>1000</ram></host>\n"
        "  </experiment>\n"
        "</experiments>\n"
    );

    write_file("tasks.csv",
        "TASK_NAME,TASK_HOST,TASK_INITIAL_SLEEP_TIME,TASK_RUN_TIME,TASK_RAM,TASK_NETWORK_TIME,TASK_DEPENDENCY\n"
        "ZeroTask,HOST_0,0,0,0,0,\n"
    );

    auto experiments = parsers::load_experiments_from_xml(test_dir + "/config.xml");
    auto experiment = parsers::get_experiment_config(experiments, "test");
    auto tasks = parsers::parse_tasks_csv(test_dir + "/tasks.csv");

    parsers::validate_task_dependencies(tasks);

    simulator::TaskSimulator sim(experiment, std::move(tasks));

    // Should complete successfully (task uses no resources)
    EXPECT_NO_THROW(sim.run());
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
