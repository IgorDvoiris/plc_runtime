# plugins/system_monitor/CMakeLists.txt

cmake_minimum_required(VERSION 3.16)
project(iec_runtime_plugin)

set(CMAKE_CXX_STANDARD 20)

# ── SHARED LIBRARY + PIC ─────────────────────────────────────
add_library(iec_runtime SHARED
    src/IecProgram.cpp
    src/MatiecLoader.cpp
    src/IecRuntimePlugin.cpp
)

# Position Independent Code — обязателен для .so
set_target_properties(iec_runtime PROPERTIES
    POSITION_INDEPENDENT_CODE ON          # -fPIC
    OUTPUT_NAME "iec_runtime"          # → libsystem_monitor.so
)

target_include_directories(iec_runtime
    PRIVATE
        include
        ${PROJECT_SOURCE_DIR}/../../     # путь до core/
)

target_compile_options(iec_runtime PRIVATE
    -Wall
    -Wextra
    -fvisibility=hidden                  # экспортируем только extern "C"
)

target_link_libraries(iec_runtime PRIVATE dl)
