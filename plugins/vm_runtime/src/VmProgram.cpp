// plugins/iec_runtime/src/IecProgram.cpp
#include "VmProgram.hpp"
#include <iostream>
#include "vm.hpp"
#include "serializer.hpp"
#include <iostream>

VmProgram::VmProgram(const std::string& instanceName, VmRawBuffer& rawBuf)
    : instanceName_(instanceName), rawBuf_(rawBuf) {}

bool VmProgram::load(const std::string& soPath) {
    std::cout << "[VmProgram:" << instanceName_ << "] loading " << soPath << "\n";
    try {
        vm_ = std::make_unique<VirtualMachine>();
        auto bcFile = Serializer::load(soPath);
 
        for (auto& pou : bcFile.pous) {
            vm_->loadPOU(pou);
        }

        //ctrl_ = std::make_unique<ScanCycleManager>(vm_.get(), instanceName_, 10);
        std::cout << "VM PLC Runtime loaded. Press Ctrl+C to stop.\n";
        return true;
    } catch (const std::exception& ex) {
        std::cerr << "VM PLC Runtime fatal: " << ex.what() << "\n";
        return false;
    }
}

void VmProgram::tick() {
    std::cout << "VM PLC Runtime begin: " << instanceName_ <<  "\n";            
    vm_->executeCycle(instanceName_);
}

VmProgram::~VmProgram() = default;