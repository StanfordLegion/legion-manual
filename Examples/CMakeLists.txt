cmake_minimum_required(VERSION 3.7)
project(LegionManualExamples)

find_package(Legion REQUIRED)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

add_subdirectory(Coherence)
add_subdirectory(ControlReplication)
add_subdirectory(Mapping)
add_subdirectory(Partitions)
add_subdirectory(Regions)
add_subdirectory(Tasks)
