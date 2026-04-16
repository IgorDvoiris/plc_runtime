#pragma once

class SystemBus;

class IComponent {
public:
    virtual ~IComponent() = default;

    virtual void init(SystemBus& bus)  = 0;
    virtual void start()               = 0;
    virtual void stop()                = 0;
    virtual const char* name() const   = 0;
};
