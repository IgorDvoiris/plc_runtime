// plugins/vm_runtime/include/VmProgram.hpp
#pragma once
#include "VmRawBuffer.hpp"
#include "io_mapper.hpp"
#include "core/IScheduler.hpp"
#include <string>
#include <memory>

class VirtualMachine; 
class ScanCycleManager;

class VmProgram : public ITickable {
public:
    explicit VmProgram(const std::string& instanceName, VmRawBuffer& rawBuf);
    ~VmProgram();

    bool load(const std::string& soPath);
    void tick() override;

    const std::string& instanceName() const { return instanceName_; }
    bool               isLoaded()     const { return true; }

    uint64_t      tickCount_ = 0;
private:
    std::string   instanceName_;
    VmRawBuffer& rawBuf_;

    std::unique_ptr<IOMapper> iomapper_;
    std::unique_ptr<VirtualMachine> vm_;
};
