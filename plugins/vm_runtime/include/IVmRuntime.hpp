// plugins/vm_runtime/include/IVmRuntime.hpp
#pragma once
#include <string>

class IVmRuntime {
public:
    virtual ~IVmRuntime() = default;
    virtual bool   loadProgram(const std::string& byteCodePath,
                               const std::string& instanceName,
                               const std::string& taskName) = 0;
    virtual void   unloadAll() = 0;
    virtual size_t programCount() const = 0;
};
