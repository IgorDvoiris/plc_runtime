#pragma once
#include "IComponent.hpp"
#include "TaskManager.hpp"

class TaskManagerComponent : public IComponent {
public:
    void        init(SystemBus& bus) override;
    void        start()              override;
    void        stop()               override;
    const char* name() const         override { return "TaskManagerComponent"; }
    TaskManager& taskManager() { return taskManager_; }

private:
    TaskManager  taskManager_;
    IScheduler*  scheduler_ = nullptr;
};
