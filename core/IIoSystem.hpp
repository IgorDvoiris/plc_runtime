// core/IIoSystem.hpp
#pragma once

class IIoSystem {
public:
    virtual ~IIoSystem() = default;
    virtual void start() = 0;
    virtual void stop()  = 0;
};
