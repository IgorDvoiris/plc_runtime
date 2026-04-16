// plugins/iec_runtime/include/IecProgram.hpp
#pragma once
#include "MatiecLoader.hpp"
#include "IecRawBuffer.hpp"
#include "core/IScheduler.hpp"
#include <string>

class IecProgram : public ITickable {
public:
    explicit IecProgram(const std::string& instanceName, IecRawBuffer& rawBuf);
    ~IecProgram() = default;

    bool load(const std::string& soPath);
    void tick() override;

    const std::string& instanceName() const { return instanceName_; }
    bool               isLoaded()     const { return loader_.isLoaded(); }

    uint64_t      tickCount_ = 0;
private:
    std::string   instanceName_;
    IecRawBuffer& rawBuf_;
    MatiecLoader  loader_;
};
