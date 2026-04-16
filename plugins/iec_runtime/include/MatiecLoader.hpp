// plugins/iec_runtime/include/MatiecLoader.hpp
#pragma once
#include "IecRawBuffer.hpp"
#include <string>
#include <cstdint>

static constexpr size_t GLUE_BUFFER_SIZE = 1024;

class MatiecLoader {
public:
    using ConfigInitFn = void (*)();
    using ConfigRunFn  = void (*)(uint64_t);
    using GlueVarsFn   = void (*)();
    using UpdateTimeFn = void (*)();

    using IEC_BOOL  = uint8_t;
    using IEC_BYTE  = uint8_t;
    using IEC_INT   = int16_t;
    using IEC_DINT  = int32_t;
    using IEC_LINT  = int64_t;

    ~MatiecLoader();

    bool load(const std::string& soPath, IecRawBuffer& rawBuf);
    void run(uint64_t tickCount);
    void syncOutputsToBuffer(IecRawBuffer& rawBuf);
    void copyInputsFromBuffer(const IecRawBuffer& rawBuf);
    void unload();

    bool isLoaded() const { return handle_ != nullptr; }

private:
    bool resolveSymbols();

    void*        handle_     = nullptr;
    ConfigInitFn configInit_ = nullptr;
    ConfigRunFn  configRun_  = nullptr;
    GlueVarsFn   glueVars_   = nullptr;
    UpdateTimeFn updateTime_ = nullptr;

    IEC_BOOL* (*boolInput_) [8] = nullptr;
    IEC_BOOL* (*boolOutput_)[8] = nullptr;

    IEC_BYTE* byteInput_  = nullptr;
    IEC_BYTE* byteOutput_ = nullptr;
    IEC_INT*  intInput_   = nullptr;
    IEC_INT*  intOutput_  = nullptr;
    IEC_DINT* dintInput_  = nullptr;
    IEC_DINT* dintOutput_ = nullptr;
    IEC_LINT* lintInput_  = nullptr;
    IEC_LINT* lintOutput_ = nullptr;
};
