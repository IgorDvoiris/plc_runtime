#pragma once
#include "ISystemMonitor.hpp"
#include <unordered_map>
#include <mutex>
#include <optional>

class WatchdogChecker {
public:
    struct Entry {
        uint64_t expectedIntervalMs;
        uint64_t toleranceMs;
        uint64_t lastTickMs = 0;
        bool     firstTick  = true;
    };

    void watchTask(const std::string& taskName,
                   uint64_t expectedIntervalMs,
                   uint64_t toleranceMs);

    std::optional<TaskTimingInfo> notifyTickStart(const std::string& taskName,
                                                  uint64_t nowMs);
private:
    std::unordered_map<std::string, Entry> entries_;
    std::mutex mutex_;
};
