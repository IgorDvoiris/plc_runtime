#include "Scheduler.hpp"
#include <algorithm>

void Scheduler::addTickable(ITickable* tickable, uint32_t intervalMs) {
    entries_.push_back(Entry{tickable, intervalMs, 0});
}

void Scheduler::removeTickable(ITickable* tickable) {
    entries_.erase(
        std::remove_if(entries_.begin(), entries_.end(),
            [tickable](const Entry& e) { return e.tickable == tickable; }),
        entries_.end()
    );
}

void Scheduler::tick(uint64_t nowMs) {
    for (auto& entry : entries_) {
        const bool due = (entry.intervalMs == 0) ||
                         (nowMs - entry.lastTickMs >= entry.intervalMs);
        if (due) {
            entry.tickable->tick();
            entry.lastTickMs = nowMs;
        }
    }
}
