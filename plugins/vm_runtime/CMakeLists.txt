# plugins/simulator/CMakeLists.txt

cmake_minimum_required(VERSION 3.16)
project(vm_runtime_plugin)

set(CMAKE_CXX_STANDARD 20)

# ── SHARED LIBRARY + PIC ─────────────────────────────────────
add_library(vm_runtime SHARED
    src/execution_context.cpp
    src/serializer.cpp
    src/value.cpp
    src/vm.cpp
    src/VmProgram.cpp
    src/VmRuntimePlugin.cpp    
)

# Position Independent Code — обязателен для .so
set_target_properties(vm_runtime PROPERTIES
    POSITION_INDEPENDENT_CODE ON          # -fPIC
    OUTPUT_NAME "vm_runtime"          # → libsystem_monitor.so
)

target_include_directories(vm_runtime
    PRIVATE
        include
        compiler
        include/platform
        ${PROJECT_SOURCE_DIR}/../../     # путь до core/
)

target_compile_options(vm_runtime PRIVATE
    -Wall
    -Wextra
    -fvisibility=hidden                  # экспортируем только extern "C"
)

target_link_libraries(vm_runtime PRIVATE dl)
