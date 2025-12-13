cmake_minimum_required(VERSION 3.15)
project(TaskSimulator CXX)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# tinyxml2 configuration
if(WIN32)
    # On Windows, always use local tinyxml2 from external directory
    message(STATUS "Windows detected - using external/tinyxml2")
    add_library(tinyxml2 STATIC external/tinyxml2/tinyxml2.cpp)
    target_include_directories(tinyxml2 PUBLIC external/tinyxml2)
    set(TINYXML2_LIBRARIES tinyxml2)
else()
    # On Linux/Unix, try to use system-installed tinyxml2
    find_package(tinyxml2 QUIET)

    if(tinyxml2_FOUND)
        message(STATUS "Using system-installed tinyxml2")
        set(TINYXML2_LIBRARIES tinyxml2::tinyxml2)
    else()
        # Fallback to external tinyxml2 if system version not found
        message(STATUS "System tinyxml2 not found - using external/tinyxml2")
        add_library(tinyxml2 STATIC external/tinyxml2/tinyxml2.cpp)
        target_include_directories(tinyxml2 PUBLIC external/tinyxml2)
        set(TINYXML2_LIBRARIES tinyxml2)
    endif()
endif()

# spdlog configuration
if(WIN32)
    # On Windows, always use local spdlog from external directory
    message(STATUS "Windows detected - using external/spdlog")
    add_subdirectory(external/spdlog)
    set(SPDLOG_LIBRARIES spdlog::spdlog)
else()
    # On Linux/Unix, try to use system-installed spdlog
    find_package(spdlog QUIET)

    if(spdlog_FOUND)
        message(STATUS "Using system-installed spdlog")
        set(SPDLOG_LIBRARIES spdlog::spdlog)
    else()
        # Fallback to external spdlog if system version not found
        message(STATUS "System spdlog not found - using external/spdlog")
        add_subdirectory(external/spdlog)
        set(SPDLOG_LIBRARIES spdlog::spdlog)
    endif()
endif()

# simcpp20 configuration (header-only library)
if(WIN32)
    # On Windows, always use local simcpp20 from external directory
    message(STATUS "Windows detected - using external/simcpp20")
    include_directories(${PROJECT_SOURCE_DIR}/external/simcpp20/include)
else()
    # On Linux/Unix, try to use system-installed simcpp20
    find_path(SIMCPP20_INCLUDE_DIR
        NAMES fschuetz04/simcpp20.hpp
        PATHS /usr/include /usr/local/include
        DOC "simcpp20 include directory"
    )

    if(SIMCPP20_INCLUDE_DIR)
        message(STATUS "Using system-installed simcpp20 at ${SIMCPP20_INCLUDE_DIR}")
        include_directories(${SIMCPP20_INCLUDE_DIR})
    else()
        # Fallback to external simcpp20 if system version not found
        message(STATUS "System simcpp20 not found - using external/simcpp20")
        include_directories(${PROJECT_SOURCE_DIR}/external/simcpp20/include)
    endif()
endif()

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/include)

# Parsers library
add_library(parsers STATIC
    src/config_parser.cpp
    src/csv_parser.cpp
)
target_link_libraries(parsers ${TINYXML2_LIBRARIES} ${SPDLOG_LIBRARIES})

# Simulator library (using SimCpp20)
add_library(simulator_lib STATIC
    src/simulator.cpp
)
target_link_libraries(simulator_lib ${SPDLOG_LIBRARIES})

# Main executable
add_executable(task_simulator
    src/main.cpp
)

target_link_libraries(task_simulator
    parsers
    simulator_lib
    ${SPDLOG_LIBRARIES}
)

# Installation
install(TARGETS task_simulator
    RUNTIME DESTINATION bin
)

# Enable testing
enable_testing()

# Google Test configuration
if(WIN32)
    # On Windows, use pre-built googletest from external directory
    message(STATUS "Windows detected - using external/googletest")
    add_subdirectory(external/googletest)
    set(GTEST_LIBRARIES gtest gtest_main)
    set(GTEST_BOTH_LIBRARIES gtest gtest_main)
else()
    # On other platforms, try to use system-installed GTest
    find_package(GTest QUIET)

    if(GTest_FOUND)
        message(STATUS "Using system-installed GTest")
        set(GTEST_LIBRARIES GTest::GTest GTest::Main)
        set(GTEST_BOTH_LIBRARIES GTest::GTest GTest::Main)
    else()
        # Fallback to external googletest if system version not found
        message(STATUS "System GTest not found - using external/googletest")
        add_subdirectory(external/googletest)
        set(GTEST_LIBRARIES gtest gtest_main)
        set(GTEST_BOTH_LIBRARIES gtest gtest_main)
    endif()
endif()

include(GoogleTest)

# Test executable
add_executable(edge_cases_test
    tests/edge_cases_test.cpp
)

target_link_libraries(edge_cases_test
    parsers
    simulator_lib
    ${GTEST_BOTH_LIBRARIES}
)

# Discover tests
gtest_discover_tests(edge_cases_test)

# Print build information
message(STATUS "")
message(STATUS "========== Build Configuration ==========")
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ Standard: C++${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "")
message(STATUS "Dependencies:")
if(WIN32)
    message(STATUS "  tinyxml2:  external/tinyxml2")
    message(STATUS "  spdlog:    external/spdlog")
    message(STATUS "  simcpp20:  external/simcpp20")
    message(STATUS "  googletest: external/googletest")
else()
    if(tinyxml2_FOUND)
        message(STATUS "  tinyxml2:  system")
    else()
        message(STATUS "  tinyxml2:  external/tinyxml2")
    endif()

    if(spdlog_FOUND)
        message(STATUS "  spdlog:    system")
    else()
        message(STATUS "  spdlog:    external/spdlog")
    endif()

    if(SIMCPP20_INCLUDE_DIR)
        message(STATUS "  simcpp20:  system (${SIMCPP20_INCLUDE_DIR})")
    else()
        message(STATUS "  simcpp20:  external/simcpp20")
    endif()

    if(GTest_FOUND)
        message(STATUS "  googletest: system")
    else()
        message(STATUS "  googletest: external/googletest")
    endif()
endif()
message(STATUS "==========================================")
