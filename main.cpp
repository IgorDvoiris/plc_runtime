// main.cpp
#include "core/ComponentManager.hpp"
#include "core/SystemBus.hpp"
#include "core/SchedulerComponent.hpp"
#include "core/TaskManagerComponent.hpp"
#include "core/ProcessImage.hpp"
#include "core/IoBuffer.hpp"
#include "simulation/MotorSimulation.hpp"
#include "simulation/MotorControlTask.hpp"
#include "core/PluginLoader.hpp"
#include "plugins/system_monitor/include/ISystemMonitor.hpp"
#include "plugins/iec_runtime/include/IIecRuntime.hpp"
#include "plugins/vm_runtime/include/IVmRuntime.hpp"

#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include <csignal>
#include <dlfcn.h>

namespace {
    std::atomic<bool> g_running{true};
    void signalHandler(int) { g_running = false; }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Ошибка: Неверное количество аргументов!" << std::endl;
        return 1;
    }
    std::signal(SIGINT, signalHandler);
    std::cout << "=== Motor Control System + IEC Runtime ===\n\n";

    // ── 1. IO инфраструктура ──────────────────────────────────────────
    ProcessImage processImage;
    IoBuffer     ioBuffer;

    // ── 2. Наблюдатель состояния мотора ───────────────────────────────
    // Логика управления — в IEC программе MotorControl.
    // MotorControlTask только читает выходы и печатает сообщения.
    MotorControlTask motorStatusTask(processImage);

    // ── 3. Компоненты runtime ─────────────────────────────────────────
    SchedulerComponent   schedulerComp;
    TaskManagerComponent taskManagerComp;

    SystemBus        bus;
    ComponentManager manager;

    manager.registerComponent(&schedulerComp);
    manager.registerComponent(&taskManagerComp);
    manager.initAll(bus);

    auto* taskManager = bus.getService<ITaskManager>();
    auto* scheduler   = bus.getService<IScheduler>();

    // ── 4. SystemMonitor ──────────────────────────────────────────────
    PluginLoader monitorLoader;
    bool         monitorLoaded = false;
    try {
        monitorLoader.load("./libsystem_monitor.so", bus);
        monitorLoaded = true;
    } catch (const std::exception& e) {
        std::cerr << "[Main] SystemMonitor: " << e.what() << "\n";
    }

    ISystemMonitor* monitor = nullptr;
    if (monitorLoaded) {
        try { monitor = bus.getService<ISystemMonitor>(); }
        catch (...) {}
    }

    if (monitor) {
        monitor->onOverrun([](const TaskTimingInfo& info) {
            std::cerr << "[ALERT] " << info.taskName
                      << " overrun=" << info.overrunMs << "ms\n";
        });
    }

    // ── 5. IEC Runtime ────────────────────────────────────────────────
#if 0
    void*    iecHandle = nullptr;
    IPlugin* iecPlugin = nullptr;

    iecHandle = dlopen("./libiec_runtime.so", RTLD_NOW | RTLD_LOCAL);
    if (!iecHandle) {
        std::cerr << "[Main] libiec_runtime.so not found: " << dlerror() << "\n";
        std::cerr << "[Main] невозможно продолжить без IEC runtime\n";
        return 1;
    }

    // Передать ProcessImage в плагин до createPlugin()
    using SetPiFn   = void(*)(ProcessImage*);
    using CreateFn  = IPlugin*(*)();
    using DestroyFn = void(*)(IPlugin*);

    auto setPi   = reinterpret_cast<SetPiFn>  (dlsym(iecHandle, "setProcessImage"));
    auto create  = reinterpret_cast<CreateFn> (dlsym(iecHandle, "createPlugin"));
    auto destroy = reinterpret_cast<DestroyFn>(dlsym(iecHandle, "destroyPlugin"));

    if (!setPi || !create || !destroy) {
        std::cerr << "[Main] dlsym failed: " << dlerror() << "\n";
        dlclose(iecHandle);
        return 1;
    }

    setPi(&processImage);
    iecPlugin = create();
    if (!iecPlugin) {
        std::cerr << "[Main] createPlugin() returned null\n";
        dlclose(iecHandle);
        return 1;
    }

    iecPlugin->init(bus);
#else
    void*    vmHandle = nullptr;
    IPlugin* vmPlugin = nullptr;

    vmHandle = dlopen("./libvm_runtime.so", RTLD_NOW | RTLD_LOCAL);
    if (!vmHandle) {
        std::cerr << "[Main] libvm_runtime.so not found: " << dlerror() << "\n";
        std::cerr << "[Main] невозможно продолжить без VM runtime\n";
        return 1;
    }

    // Передать ProcessImage в плагин до createPlugin()
    using SetPiFn   = void(*)(ProcessImage*);
    using CreateFn  = IPlugin*(*)();
    using DestroyFn = void(*)(IPlugin*);

    auto setPi   = reinterpret_cast<SetPiFn>  (dlsym(vmHandle, "setProcessImage"));
    auto create  = reinterpret_cast<CreateFn> (dlsym(vmHandle, "createPlugin"));
    auto destroy = reinterpret_cast<DestroyFn>(dlsym(vmHandle, "destroyPlugin"));

    if (!setPi || !create || !destroy) {
        std::cerr << "[Main] dlsym failed: " << dlerror() << "\n";
        dlclose(vmHandle);
        return 1;
    }

    setPi(&processImage);
    vmPlugin = create();
    if (!vmPlugin) {
        std::cerr << "[Main] createPlugin() returned null\n";
        dlclose(vmHandle);
        return 1;
    }

    vmPlugin->init(bus);
#endif
    // ── 6. Задачи ─────────────────────────────────────────────────────
    // IecTask: исполняет IEC программу MotorControl (интервал из CONFIGURATION = 20ms)
#if 0
    taskManager->createTask("IecTask", 20);
#else
    taskManager->createTask("VmTask", 20);
#endif


    // StatusTask: читает выходы и печатает сообщения (100ms достаточно)
    taskManager->createTask("StatusTask", 100);
    taskManager->addToTask("StatusTask", &motorStatusTask);

    if (monitor) {
#if 0
        monitor->watchTask("IecTask",    20,  30);
#else
        monitor->watchTask("VmTask",    20,  30);
#endif
        monitor->watchTask("StatusTask", 100, 15);

        taskManager->setTickCallback(
            [monitor](const std::string& name, uint64_t now) {
                monitor->notifyTickStart(name, now);
            });
    }
#if 0
    // Загрузить IEC программу в IecTask
    IIecRuntime* iecRuntime = nullptr;
    try { iecRuntime = bus.getService<IIecRuntime>(); }
    catch (const std::exception& e) {
        std::cerr << "[Main] IIecRuntime: " << e.what() << "\n";
        destroy(iecPlugin);
        dlclose(iecHandle);
        return 1;
    }

    if (!iecRuntime->loadProgram("./libst_logic.so", "Inst0", "IecTask")) {
        std::cerr << "[Main] loadProgram failed\n";
        destroy(iecPlugin);
        dlclose(iecHandle);
        return 1;
    }
#else
    IVmRuntime* vmRuntime = nullptr;
    try { vmRuntime = bus.getService<IVmRuntime>(); }
    catch (const std::exception& e) {
        std::cerr << "[Main] IVmRuntime: " << e.what() << "\n";
        destroy(vmPlugin);
        dlclose(vmHandle);
        return 1;
    }

    if (!vmRuntime->loadProgram(argv[1], "PumpControl", "VmTask")) {
        std::cerr << "[Main] loadProgram failed\n";
        destroy(vmPlugin);
        dlclose(vmHandle);
        return 1;
    }
#endif
    // ── 7. Start ──────────────────────────────────────────────────────
    manager.startAll();
#if 0
    iecPlugin->start();
#else
    vmPlugin->start();
#endif

    MotorSimulation simulation(ioBuffer);
    //simulation.start();

    std::cout << "\n[Main] running (Ctrl+C to stop)\n\n";

    // ── 8. Главный цикл ───────────────────────────────────────────────
    using clock = std::chrono::steady_clock;
    using ms    = std::chrono::milliseconds;

    const ms tickInterval{10};
    auto     nextTick = clock::now() + tickInterval;

    while (g_running) {
        const auto nowMs = static_cast<uint64_t>(
            std::chrono::duration_cast<ms>(
                clock::now().time_since_epoch()).count());

        // Синхронизация IO: входы из симулятора → ProcessImage
        ioBuffer.copyInputsToProcessImage(processImage);
// В main.cpp — главный цикл, после copyInputsToProcessImage():
        // Исполнение всех задач (IecTask + StatusTask)
        scheduler->tick(nowMs);

        // Синхронизация IO: выходы ProcessImage → симулятор
        ioBuffer.copyOutputsFromProcessImage(processImage);
#if 0
if (nowMs % 5) { //1000 < 10) {
    std::cout << "[DEBUG 2] t=" << nowMs << "] "
              << "btnStart=" << (int)processImage.inputs[0]    // %IX0.0
              << " btnStop="  << (int)processImage.inputs[1]   // %IX0.1
              << " fault="    << (int)processImage.inputs[2]   // %IX0.2
              << " | "
              << "motorRun="  << (int)processImage.outputs[8]  // %QX1.0
              << " green="    << (int)processImage.outputs[9]  // %QX1.1
              << " red="      << (int)processImage.outputs[10] // %QX1.2
              << "\n";
}
#endif
        std::this_thread::sleep_until(nextTick);
        nextTick += tickInterval;

        //if (!simulation.isRunning()) g_running = false;
    }

    // ── 9. Остановка ──────────────────────────────────────────────────
    std::cout << "\n[Main] shutting down...\n";
    simulation.stop();
    manager.stopAll();
    monitorLoader.unloadAll();

#if 0
    iecPlugin->stop();
    destroy(iecPlugin);
    dlclose(iecHandle);
#else
    vmPlugin->stop();
    destroy(vmPlugin);
    dlclose(vmHandle);
#endif

    std::cout << "\n=== Done ===\n";
    return 0;
}