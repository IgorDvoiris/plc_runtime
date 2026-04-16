// plugins/iec_runtime/src/IecProgram.cpp
#include "IecProgram.hpp"
#include <iostream>

IecProgram::IecProgram(const std::string& instanceName, IecRawBuffer& rawBuf)
    : instanceName_(instanceName), rawBuf_(rawBuf) {}

bool IecProgram::load(const std::string& soPath) {
    std::cout << "[IecProgram:" << instanceName_ << "] loading " << soPath << "\n";
    if (!loader_.load(soPath, rawBuf_)) {
        std::cerr << "[IecProgram:" << instanceName_ << "] load failed\n";
        return false;
    }
    std::cout << "[IecProgram:" << instanceName_ << "] ready\n";
    return true;
}

void IecProgram::tick() {
    if (!loader_.isLoaded()) return;
    // Перед run: скопировать текущие входы в буферы .so
    loader_.copyInputsFromBuffer(rawBuf_);  // нужен доступ — сделаем friend или метод
    loader_.run(tickCount_++);
    // После run: синхронизировать выходы обратно в ProcessImage
    loader_.syncOutputsToBuffer(rawBuf_);
}
