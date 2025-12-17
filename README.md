# Task Simulator - C++ Discrete Event Simulation

C++20 discrete event simulator for multi-host task scheduling with resource management. Uses SimCpp20 framework.

## Features

- Multi-host task scheduling with CPU and RAM resource management
- Task dependencies and network transmission simulation
- CPU utilization statistics and performance metrics
- XML configuration and CSV task definitions
- Structured logging with spdlog

## Requirements

**Build tools:**
- CMake 3.15+
- C++20 compiler (GCC 10+, Clang 10+, MSVC 2019+)

**Dependencies (auto-detected on Linux, bundled for Windows):**
- tinyxml2, spdlog, simcpp20, googletest

**Install dependencies on Ubuntu/Debian:**
```bash
sudo apt-get install cmake g++ libtinyxml2-dev libspdlog-dev libgtest-dev
```

## Building

```bash
git submodule update --init --recursive
mkdir build && cd build
cmake ..
make
```

## Tests

```bash
./edge_cases_test
./performance_test --gtest_also_run_disabled_tests
```

## Usage

```bash
./task_simulator <experiments.xml> --experiment <name> [--verbose]
```

**Arguments:**
- `experiments.xml` - XML file with experiment configurations
- `--experiment, -e NAME` - Experiment name to run
- `--verbose, -v` - Show detailed per-host statistics

**Examples:**
```bash
# Run experiment
./task_simulator ../experiments.xml --experiment long_sleep

# Run with detailed statistics
./task_simulator ../experiments.xml -e long_sleep --verbose
```

## Output Example

```
[INFO] Starting simulation...
======================================================================
Starting simulation with 5 tasks
======================================================================
[INFO] [HOST_0] [t=0]   Task TASK_0: Started execution (CPU acquired, 2000 RAM allocated)
[INFO] [HOST_0] [t=1000] Task TASK_0: Finished execution
...
======================================================================
Simulation completed at t=23500
======================================================================

Overall Statistics:
----------------------------------------------------------------------
Total CPU cores:        2
Total CPU work time:    32500
Total CPU available:    47000 (2 cores × 23500)
Total CPU idle time:    14500
CPU utilization:        69.15%
======================================================================
```

## Project Structure

```
run_simulation/
├── CMakeLists.txt      # Build configuration
├── external/           # Bundled dependencies (fallback)
├── include/            # Headers (models, parsers, simulator, logger)
├── src/                # Implementation
│   ├── main.cpp
│   ├── config_parser.cpp
│   ├── csv_parser.cpp
│   └── simulator.cpp
├── data/               # Sample experiments
└── tests/              # Unit tests
```

## License

MIT License

Based on SimCpp20 by Felix Schütz.
