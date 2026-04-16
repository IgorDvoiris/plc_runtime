#pragma once
#include "IScheduler.hpp"
#include <vector>
#include <cstdint>

class Scheduler : public IScheduler {
public:
    void addTickable(ITickable* tickable, uint32_t intervalMs = 0) override;
    void removeTickable(ITickable* tickable) override;
    void tick(uint64_t nowMs) override;

private:
    struct Entry {
        ITickable* tickable;
        uint32_t   intervalMs;
        uint64_t   lastTickMs;
    };
    std::vector<Entry> entries_;
};
