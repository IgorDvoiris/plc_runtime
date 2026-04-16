#pragma once
#include "IScheduler.hpp"
#include <string>
#include <functional>

class ITaskManager {
public:
    virtual ~ITaskManager() = default;

    virtual ITickable& createTask(const std::string& name,
                                  uint32_t           intervalMs) = 0;

    virtual void addToTask(const std::string& taskName,
                           ITickable*         unit) = 0;

    virtual void setTickCallback(
        std::function<void(const std::string&, uint64_t)> cb) = 0;
};
