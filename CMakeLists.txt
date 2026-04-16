cmake_minimum_required(VERSION 3.20)
project(system CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(system
    main.cpp
    core/ComponentManager.cpp
    core/Scheduler.cpp
    core/SchedulerComponent.cpp
    core/TaskManager.cpp
    core/TaskManagerComponent.cpp
    core/PluginLoader.cpp
    simulation/MotorSimulation.cpp
    simulation/MotorControlTask.cpp
)

target_include_directories(system PRIVATE ${CMAKE_SOURCE_DIR})
target_compile_options(system PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(system PRIVATE pthread)
