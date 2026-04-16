#pragma once
#include <cstdint>

struct ITickable {
    virtual ~ITickable() = default;
    virtual void tick() = 0;
};

class IScheduler {
public:
    virtual ~IScheduler() = default;
    virtual void addTickable(ITickable* tickable, uint32_t intervalMs = 0) = 0;
    virtual void removeTickable(ITickable* tickable) = 0;
    virtual void tick(uint64_t nowMs) = 0;
};
