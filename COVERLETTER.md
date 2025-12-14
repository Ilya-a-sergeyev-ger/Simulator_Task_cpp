# Selection of External Dependencies

**General filters:** free to use; popularity in industry

- **simcpp20** — the closest to SimPy; practically a copy. Coroutines allow writing code that is essentially identical to Python. MIT license.
- **spdlog** — there was no logging requirement for C++; chose the industry standard. Simple to use.
- **googletest** — industry standard for testing.
- **tinyxml2** — a ready parser was available; no reason to use something more complex.

# Basic Functionality

- Simulation
- Automated tests using Googletest

# Additional Implemented Functionality

- Experiment configuration in XML
- Calculation of CPU utilization in the experiment (an obvious goal of the project)

# Limitations

- All tasks must be loaded at once from a file; to implement streaming task ingestion, refactoring is required.
