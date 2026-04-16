#include "WatchdogChecker.hpp"

void WatchdogChecker::watchTask(const std::string& taskName,
                                uint64_t expectedIntervalMs,
                                uint64_t toleranceMs) {
    std::lock_guard<std::mutex> lock(mutex_);
    entries_[taskName] = Entry{expectedIntervalMs, toleranceMs, 0, true};
}

std::optional<TaskTimingInfo>
WatchdogChecker::notifyTickStart(const std::string& taskName, uint64_t nowMs) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = entries_.find(taskName);
    if (it == entries_.end()) return std::nullopt;

    Entry& e = it->second;
    if (e.firstTick) { e.lastTickMs = nowMs; e.firstTick = false; return std::nullopt; }

    const uint64_t actual = nowMs - e.lastTickMs;
    e.lastTickMs = nowMs;

    if (actual > e.expectedIntervalMs + e.toleranceMs) {
        return TaskTimingInfo{taskName, e.expectedIntervalMs, actual,
                              actual - e.expectedIntervalMs};
    }
    return std::nullopt;
}
