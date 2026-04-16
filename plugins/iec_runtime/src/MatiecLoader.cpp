// plugins/iec_runtime/src/MatiecLoader.cpp
#include "MatiecLoader.hpp"
#include <dlfcn.h>
#include <cstring>
#include <iostream>

namespace MatiecSymbols {
    constexpr const char* CONFIG_INIT  = "config_init__";
    constexpr const char* CONFIG_RUN   = "config_run__";
    constexpr const char* GLUE_VARS    = "glueVars";
    constexpr const char* UPDATE_TIME  = "updateTime";

    constexpr const char* BOOL_INPUT   = "bool_input";
    constexpr const char* BOOL_OUTPUT  = "bool_output";
    constexpr const char* BYTE_INPUT   = "byte_input";
    constexpr const char* BYTE_OUTPUT  = "byte_output";
    constexpr const char* INT_INPUT    = "int_input";
    constexpr const char* INT_OUTPUT   = "int_output";
    constexpr const char* DINT_INPUT   = "dint_input";
    constexpr const char* DINT_OUTPUT  = "dint_output";
    constexpr const char* LINT_INPUT   = "lint_input";
    constexpr const char* LINT_OUTPUT  = "lint_output";
}

MatiecLoader::~MatiecLoader() { unload(); }

bool MatiecLoader::load(const std::string& soPath, IecRawBuffer& rawBuf) {
    if (handle_) { std::cerr << "[MatiecLoader] already loaded\n"; return false; }

    handle_ = dlopen(soPath.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!handle_) {
        std::cerr << "[MatiecLoader] dlopen failed: " << dlerror() << "\n";
        return false;
    }
    std::cout << "[MatiecLoader] dlopen: " << soPath << "\n";

    if (!resolveSymbols()) { unload(); return false; }

    copyInputsFromBuffer(rawBuf);

    std::cout << "[MatiecLoader] calling glueVars()\n";
    glueVars_();

    std::cout << "[MatiecLoader] calling config_init__()\n";
    configInit_();

    // tickCount_ убран — счётчик тиков хранится в IecProgram
    std::cout << "[MatiecLoader] ready\n";
    return true;
}

bool MatiecLoader::resolveSymbols() {
    auto resolve = [&](const char* sym, void** out, bool required) -> bool {
        *out = dlsym(handle_, sym);
        if (!*out) {
            if (required) {
                std::cerr << "[MatiecLoader] required symbol missing: " << sym << "\n";
                return false;
            }
            std::cout << "[MatiecLoader] optional missing: " << sym << "\n";
        } else {
            std::cout << "[MatiecLoader] resolved: " << sym << "\n";
        }
        return true;
    };

    if (!resolve(MatiecSymbols::CONFIG_INIT, (void**)&configInit_, true))  return false;
    if (!resolve(MatiecSymbols::CONFIG_RUN,  (void**)&configRun_,  true))  return false;
    if (!resolve(MatiecSymbols::GLUE_VARS,   (void**)&glueVars_,   true))  return false;

    resolve(MatiecSymbols::UPDATE_TIME,  (void**)&updateTime_, false);
    resolve(MatiecSymbols::BOOL_INPUT,   (void**)&boolInput_,  false);
    resolve(MatiecSymbols::BOOL_OUTPUT,  (void**)&boolOutput_, false);
    resolve(MatiecSymbols::BYTE_INPUT,   (void**)&byteInput_,  false);
    resolve(MatiecSymbols::BYTE_OUTPUT,  (void**)&byteOutput_, false);
    resolve(MatiecSymbols::INT_INPUT,    (void**)&intInput_,   false);
    resolve(MatiecSymbols::INT_OUTPUT,   (void**)&intOutput_,  false);
    resolve(MatiecSymbols::DINT_INPUT,   (void**)&dintInput_,  false);
    resolve(MatiecSymbols::DINT_OUTPUT,  (void**)&dintOutput_, false);
    resolve(MatiecSymbols::LINT_INPUT,   (void**)&lintInput_,  false);
    resolve(MatiecSymbols::LINT_OUTPUT,  (void**)&lintOutput_, false);
    return true;
}

void MatiecLoader::copyInputsFromBuffer(const IecRawBuffer& rawBuf) {
    if (boolInput_) {      
        const IEC_BOOL* src = rawBuf.boolInputData();  // плоский буфер: src[byte*8+bit]

        for (size_t byte = 0; byte < GLUE_BUFFER_SIZE; ++byte) {
            for (size_t bit = 0; bit < 8; ++bit) {
                IEC_BOOL* var = boolInput_[byte][bit];  // указатель на IEC переменную
                if (var == nullptr) continue;           // не используется в программе
                *var = src[byte * 8 + bit];             // записываем значение
            }
        }
    }
    if (byteInput_)
        std::memcpy(byteInput_, rawBuf.byteInputData(),
                    IecRawBuffer::BYTE_BUF_SIZE * sizeof(IEC_BYTE));
    if (intInput_)
        std::memcpy(intInput_,  rawBuf.intInputData(),
                    IecRawBuffer::INT_BUF_SIZE  * sizeof(IEC_INT));
    if (dintInput_)
        std::memcpy(dintInput_, rawBuf.dintInputData(),
                    IecRawBuffer::DINT_BUF_SIZE * sizeof(IEC_DINT));
    if (lintInput_)
        std::memcpy(lintInput_, rawBuf.lintInputData(),
                    IecRawBuffer::LINT_BUF_SIZE * sizeof(IEC_LINT));
}

void MatiecLoader::syncOutputsToBuffer(IecRawBuffer& rawBuf) {
    if (boolOutput_) {      
        IEC_BOOL* dst = rawBuf.boolOutputData();  // плоский буфер: src[byte*8+bit]

        for (size_t byte = 0; byte < GLUE_BUFFER_SIZE; ++byte) {
            for (size_t bit = 0; bit < 8; ++bit) {
                IEC_BOOL* var = boolOutput_[byte][bit];  // указатель на IEC переменную
                if (var == nullptr) continue;           // не используется в программе
                dst[byte * 8 + bit] = *var;             // записываем значение
            }
        }
    }

    if (byteOutput_)
        std::memcpy(rawBuf.byteOutputData(), byteOutput_,
                    IecRawBuffer::BYTE_BUF_SIZE * sizeof(IEC_BYTE));
    if (intOutput_)
        std::memcpy(rawBuf.intOutputData(),  intOutput_,
                    IecRawBuffer::INT_BUF_SIZE  * sizeof(IEC_INT));
    if (dintOutput_)
        std::memcpy(rawBuf.dintOutputData(), dintOutput_,
                    IecRawBuffer::DINT_BUF_SIZE * sizeof(IEC_DINT));
    if (lintOutput_)
        std::memcpy(rawBuf.lintOutputData(), lintOutput_,
                    IecRawBuffer::LINT_BUF_SIZE * sizeof(IEC_LINT));
}

void MatiecLoader::run(uint64_t tickCount) {
    if (!configRun_) return;
    // updateTime() обновляет __CURRENT_TIME для таймеров TON/TOF/TP
    if (updateTime_) updateTime_();
    configRun_(tickCount);
}

void MatiecLoader::unload() {
    if (!handle_) return;
    configInit_ = nullptr; configRun_  = nullptr;
    glueVars_   = nullptr; updateTime_ = nullptr;
    boolInput_  = nullptr; boolOutput_ = nullptr;
    byteInput_  = nullptr; byteOutput_ = nullptr;
    intInput_   = nullptr; intOutput_  = nullptr;
    dintInput_  = nullptr; dintOutput_ = nullptr;
    lintInput_  = nullptr; lintOutput_ = nullptr;
    dlclose(handle_);
    handle_ = nullptr;
    std::cout << "[MatiecLoader] unloaded\n";
}