#include "TaskManager.hpp"
#include <stdexcept>
#include <iostream>
#include <chrono>

Task::Task(std::string name, uint32_t intervalMs)
    : name_(std::move(name)), intervalMs_(intervalMs) {}

void Task::addUnit(ITickable* unit) { units_.push_back(unit); }

void Task::setTickCallback(std::function<void(const std::string&, uint64_t)> cb) {
    tickCb_ = std::move(cb);
}

void Task::tick() {
    if (tickCb_) {
        const auto nowMs = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count());
        tickCb_(name_, nowMs);
    }
    for (auto* unit : units_) unit->tick();
}

void TaskManager::setTickCallback(
    std::function<void(const std::string&, uint64_t)> cb) {
    tickCb_ = cb;
    for (auto& t : tasks_) t->setTickCallback(cb);
}

ITickable& TaskManager::createTask(const std::string& name, uint32_t intervalMs) {
    if (findTask(name)) throw std::runtime_error("Task exists: " + name);
    tasks_.push_back(std::make_unique<Task>(name, intervalMs));
    std::cout << "[TaskManager] created '" << name << "' " << intervalMs << "ms\n";
    if (tickCb_) tasks_.back()->setTickCallback(tickCb_);
    return *tasks_.back();
}

void TaskManager::addToTask(const std::string& taskName, ITickable* unit) {
    Task* t = findTask(taskName);
    if (!t) throw std::runtime_error("Task not found: " + taskName);
    t->addUnit(unit);
}

void TaskManager::bindScheduler(IScheduler* scheduler) {
    scheduler_ = scheduler;
    for (auto& t : tasks_) {
        scheduler_->addTickable(t.get(), t->intervalMs());
        std::cout << "[TaskManager] bound '" << t->taskName() << "'\n";
    }
}

Task* TaskManager::findTask(const std::string& name) {
    for (auto& t : tasks_)
        if (t->taskName() == name) return t.get();
    return nullptr;
}
