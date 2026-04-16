// plugins/iec_runtime/include/IIecRuntime.hpp
#pragma once
#include <string>

class IIecRuntime {
public:
    virtual ~IIecRuntime() = default;
    virtual bool   loadProgram(const std::string& soPath,
                               const std::string& instanceName,
                               const std::string& taskName) = 0;
    virtual void   unloadAll() = 0;
    virtual size_t programCount() const = 0;
};
