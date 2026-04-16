#pragma once
#include "ISystemMonitor.hpp"
#include "WatchdogChecker.hpp"
#include "core/IPlugin.hpp"
#include "core/SystemBus.hpp"
#include <vector>
#include <mutex>

#ifdef __GNUC__
#  define PLUGIN_EXPORT __attribute__((visibility("default")))
#else
#  define PLUGIN_EXPORT
#endif

extern "C" {
    PLUGIN_EXPORT IPlugin* createPlugin();
    PLUGIN_EXPORT void     destroyPlugin(IPlugin* plugin);
}

class SystemMonitorPlugin final : public IPlugin, public ISystemMonitor {
public:
    void init(SystemBus& bus) override;
    void start()             override;
    void stop()              override;
    const char* name() const override { return "SystemMonitor"; }

    void watchTask(const std::string& taskName,
                   uint64_t expectedIntervalMs,
                   uint64_t toleranceMs) override;
    void notifyTickStart(const std::string& taskName, uint64_t nowMs) override;
    void onOverrun(WatchdogCallback callback) override;

private:
    WatchdogChecker               checker_;
    std::vector<WatchdogCallback> callbacks_;
    std::mutex                    callbackMutex_;
};
