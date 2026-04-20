// vm.hpp
#pragma once
#include "execution_context.hpp"
#include "codegen.hpp"
#include "opcodes.hpp"
#include <unordered_map>
#include <functional>
#include <string>

using IOReadFn  = std::function<Value(uint16_t)>;
using IOWriteFn = std::function<void(uint16_t, const Value&)>;

class VirtualMachine {
public:
    VirtualMachine() = default;

    void loadPOU(const POUBytecode& pou);
    void setIOCallbacks(IOReadFn reader, IOWriteFn writer);
    void executeCycle(const std::string& programName);
    Value getVar(const std::string& pouName,
                 const std::string& varName) const;

private:
    std::unordered_map<std::string, POUBytecode>    pous_;
    std::unordered_map<std::string, ExecutionFrame> frameCache_;
    IOReadFn  ioRead_;
    IOWriteFn ioWrite_;

    void     initFrame(ExecutionFrame& frame, const POUBytecode& pou);
    void     execute(ExecutionFrame& frame,
                     const std::vector<uint8_t>& code);

    Value    fetchOperand(ExecutionFrame& frame,
                          const std::vector<uint8_t>& code);
    uint16_t fetchVarIdx (ExecutionFrame& frame,
                          const std::vector<uint8_t>& code);
    uint16_t fetchU16    (ExecutionFrame& frame,
                          const std::vector<uint8_t>& code);
    uint32_t fetchU32    (ExecutionFrame& frame,
                          const std::vector<uint8_t>& code);

    void callFunctionBlock(ExecutionFrame& callerFrame, uint16_t fbIdx);
};
