# plugins/system_monitor/CMakeLists.txt

cmake_minimum_required(VERSION 3.16)
project(system_monitor_plugin)

set(CMAKE_CXX_STANDARD 20)

# ── SHARED LIBRARY + PIC ─────────────────────────────────────
add_library(system_monitor SHARED
    src/SystemMonitorPlugin.cpp
    src/WatchdogChecker.cpp
)

# Position Independent Code — обязателен для .so
set_target_properties(system_monitor PROPERTIES
    POSITION_INDEPENDENT_CODE ON          # -fPIC
    OUTPUT_NAME "system_monitor"          # → libsystem_monitor.so
)

target_include_directories(system_monitor
    PRIVATE
        include
        ${PROJECT_SOURCE_DIR}/../../     # путь до core/
)

target_compile_options(system_monitor PRIVATE
    -Wall
    -Wextra
    -fvisibility=hidden                  # экспортируем только extern "C"
)
