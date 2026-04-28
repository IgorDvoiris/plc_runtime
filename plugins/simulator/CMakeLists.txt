# plugins/simulator/CMakeLists.txt

cmake_minimum_required(VERSION 3.16)
project(simulator_plugin)

set(CMAKE_CXX_STANDARD 20)

# ── SHARED LIBRARY + PIC ─────────────────────────────────────
add_library(simulator SHARED
    src/GpioSimulatorPlugin.cpp
)

# Position Independent Code — обязателен для .so
set_target_properties(simulator PROPERTIES
    POSITION_INDEPENDENT_CODE ON          # -fPIC
    OUTPUT_NAME "simulator"          # → libsystem_monitor.so
)

target_include_directories(simulator
    PRIVATE
        include
        ${PROJECT_SOURCE_DIR}/../../     # путь до core/
)

target_compile_options(simulator PRIVATE
    -Wall
    -Wextra
    -fvisibility=hidden                  # экспортируем только extern "C"
)

target_link_libraries(simulator PRIVATE dl)
