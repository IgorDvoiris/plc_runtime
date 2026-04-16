#pragma once
#include "IComponent.hpp"
#include "Scheduler.hpp"

class SchedulerComponent : public IComponent {
public:
    void        init(SystemBus& bus) override;
    void        start()              override;
    void        stop()               override;
    const char* name() const         override { return "SchedulerComponent"; }
    Scheduler& scheduler() { return scheduler_; }

private:
    Scheduler scheduler_;
};
