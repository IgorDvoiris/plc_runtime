#include "SystemMonitorPlugin.hpp"
#include <iostream>

void SystemMonitorPlugin::init(SystemBus& bus) {
    bus.registerService<ISystemMonitor>(this);
    std::cout << "[SystemMonitor] init\n";
}
void SystemMonitorPlugin::start() { std::cout << "[SystemMonitor] start\n"; }
void SystemMonitorPlugin::stop()  { std::cout << "[SystemMonitor] stop\n"; }

void SystemMonitorPlugin::watchTask(const std::string& taskName,
                                    uint64_t expectedIntervalMs,
                                    uint64_t toleranceMs) {
    checker_.watchTask(taskName, expectedIntervalMs, toleranceMs);
    std::cout << "[SystemMonitor] watching '" << taskName << "' "
              << expectedIntervalMs << "ms ±" << toleranceMs << "ms\n";
}

void SystemMonitorPlugin::notifyTickStart(const std::string& taskName,
                                          uint64_t nowMs) {
    auto result = checker_.notifyTickStart(taskName, nowMs);
    if (result.has_value()) {
        const auto& info = result.value();
        std::cout << "[SystemMonitor] OVERRUN '" << info.taskName
                  << "' expected=" << info.expectedIntervalMs
                  << "ms actual=" << info.actualIntervalMs
                  << "ms overrun=" << info.overrunMs << "ms\n";
        std::lock_guard<std::mutex> lock(callbackMutex_);
        for (auto& cb : callbacks_) cb(info);
    }
}

void SystemMonitorPlugin::onOverrun(WatchdogCallback callback) {
    std::lock_guard<std::mutex> lock(callbackMutex_);
    callbacks_.push_back(std::move(callback));
}

extern "C" {
    PLUGIN_EXPORT IPlugin* createPlugin()              { return new SystemMonitorPlugin(); }
    PLUGIN_EXPORT void     destroyPlugin(IPlugin* p)   { delete p; }
}
