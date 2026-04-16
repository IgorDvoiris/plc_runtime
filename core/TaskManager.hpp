#pragma once
#include "ITaskManager.hpp"
#include <vector>
#include <memory>
#include <string>
#include <functional>

class Task : public ITickable {
public:
    explicit Task(std::string name, uint32_t intervalMs);
    void addUnit(ITickable* unit);
    void setTickCallback(std::function<void(const std::string&, uint64_t)> cb);
    void tick() override;
    const std::string& taskName()   const { return name_; }
    uint32_t           intervalMs() const { return intervalMs_; }

private:
    std::string  name_;
    uint32_t     intervalMs_;
    std::vector<ITickable*> units_;
    std::function<void(const std::string&, uint64_t)> tickCb_;
};

class TaskManager : public ITaskManager {
public:
    ITickable& createTask(const std::string& name, uint32_t intervalMs) override;
    void addToTask(const std::string& taskName, ITickable* unit) override;
    void setTickCallback(std::function<void(const std::string&, uint64_t)> cb) override;
    void bindScheduler(IScheduler* scheduler);

private:
    Task* findTask(const std::string& name);
    std::vector<std::unique_ptr<Task>> tasks_;
    IScheduler* scheduler_ = nullptr;
    std::function<void(const std::string&, uint64_t)> tickCb_;
};
