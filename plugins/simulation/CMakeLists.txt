# plugins/simulation/CMakeLists.txt

cmake_minimum_required(VERSION 3.16)
project(simulation_plugin)

set(CMAKE_CXX_STANDARD 20)

# ── SHARED LIBRARY + PIC ─────────────────────────────────────
add_library(simulation SHARED
    src/SimulationPlugin.cpp
)

# Position Independent Code — обязателен для .so
set_target_properties(simulation PROPERTIES
    POSITION_INDEPENDENT_CODE ON          # -fPIC
    OUTPUT_NAME "simulation"              # → libsimulation.so
)

target_include_directories(simulation
    PRIVATE
        include
        ${PROJECT_SOURCE_DIR}/../../     # путь до core/
        ${PROJECT_SOURCE_DIR}/../vm_runtime/include/
        ${PROJECT_SOURCE_DIR}/../vm_runtime/include/compiler/
)

target_compile_options(simulation PRIVATE
    -Wall
    -Wextra
    -fvisibility=hidden                  # экспортируем только extern "C"
)
