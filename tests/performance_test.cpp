#include <gtest/gtest.h>
#include "../include/simulator.hpp"
#include "../include/models.h"
#include "../include/logger.hpp"
#include <vector>
#include <string>

class PerformanceTest : public ::testing::Test {
protected:
    models::ExperimentConfig generate_config(size_t num_hosts, int ram_per_host = 10000) {
        models::ExperimentConfig config;
        config.tasks_csv_path = "generated";

        for (size_t i = 0; i < num_hosts; ++i) {
            std::string host_id = "HOST_" + std::to_string(i);
            models::HostConfig host_config{4, ram_per_host};
            config.hosts[host_id] = host_config;
        }

        config.validate(true);

        return config;
    }

    std::vector<models::Task> generate_ping_pong_tasks(size_t num_tasks, size_t num_hosts) {
        std::vector<models::Task> tasks;
        tasks.reserve(num_tasks);

        for (size_t i = 0; i < num_tasks; ++i) {
            size_t host_idx = i % num_hosts;
            std::string task_name = "Task_" + std::to_string(i);
            std::string host_name = "HOST_" + std::to_string(host_idx);

            std::vector<std::string> dependencies;
            if (i >= 2) {
                dependencies.push_back("Task_" + std::to_string(i - 2));
            }

            models::Task task{
                task_name,
                host_name,
                0,
                10,
                100,
                5,
                dependencies,
                {},
                i,
                0
            };

            tasks.push_back(task);
        }

        return tasks;
    }

    template<typename Func>
    int64_t measure_time(const std::string& operation_name,Func&& func) {
        auto start = std::chrono::high_resolution_clock::now();
        func();
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        logger::info("{} took: {} microseconds", operation_name, duration.count());
        return duration.count();
    }
};

TEST_F(PerformanceTest, PingPong_1000_Tasks_10_Hosts) {
    auto config = generate_config(10);
    auto tasks = generate_ping_pong_tasks(1000, 10);

    simulator::TaskSimulator sim;

    EXPECT_NO_THROW(measure_time("Initialization", [&]() {
        sim.init(config, std::move(tasks));
    }));

    EXPECT_NO_THROW(sim.run());
}

TEST_F(PerformanceTest, PingPong_10000_Tasks_50_Hosts) {
    auto config = generate_config(50);
    auto tasks = generate_ping_pong_tasks(10000, 50);

    simulator::TaskSimulator sim;

    EXPECT_NO_THROW(measure_time("Initialization", [&]() {
        sim.init(config, std::move(tasks));
    }));

    EXPECT_NO_THROW(sim.run());
}

TEST_F(PerformanceTest, DISABLED_PingPong_1M_Tasks_100_Hosts) {
    auto config = generate_config(100);
    auto tasks = generate_ping_pong_tasks(1000000, 100);

    simulator::TaskSimulator sim;
    EXPECT_NO_THROW(measure_time("Initialization", [&]() {
        sim.init(config, std::move(tasks));
    }));

    EXPECT_NO_THROW(sim.run());
}
