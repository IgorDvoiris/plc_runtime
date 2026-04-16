#pragma once
#include <cstdint>
#include <string>
#include <functional>

struct TaskTimingInfo {
    std::string taskName;
    uint64_t    expectedIntervalMs;
    uint64_t    actualIntervalMs;
    uint64_t    overrunMs;
};

using WatchdogCallback = std::function<void(const TaskTimingInfo&)>;

class ISystemMonitor {
public:
    virtual ~ISystemMonitor() = default;
    virtual void watchTask(const std::string& taskName,
                           uint64_t expectedIntervalMs,
                           uint64_t toleranceMs) = 0;
    virtual void notifyTickStart(const std::string& taskName,
                                 uint64_t nowMs) = 0;
    virtual void onOverrun(WatchdogCallback callback) = 0;
};
