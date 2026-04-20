// vm.hpp
#pragma once
#include "execution_context.hpp"
#include "../compiler/codegen.hpp"
#include "../compiler/opcodes.hpp"
#include <unordered_map>
#include <functional>
#include <string>
#include <vector>

using IOReadFn  = std::function<Value(uint16_t)>;
using IOWriteFn = std::function<void(uint16_t, const Value&)>;

class VirtualMachine {
public:
    static constexpr uint16_t INVALID_VAR_INDEX = 0xFFFF;

    VirtualMachine() = default;

    void  loadPOU       (const POUBytecode& pou);
    void  setIOCallbacks(IOReadFn reader, IOWriteFn writer);
    void  executeCycle  (const std::string& programName);

    // ── Доступ к переменным ───────────────────────────────────

    // Поиск индекса переменной по имени (вызывается один раз при init)
    uint16_t resolveVarIndex(const std::string& pouName,
                             const std::string& varName) const;

    // Чтение/запись по кэшированному индексу (hot path)
    void  setVarByIndex(const std::string& pouName,
                        uint16_t index,
                        const Value& value);

    Value getVarByIndex(const std::string& pouName,
                        uint16_t index) const;

    // Чтение/запись по имени (удобно для отладки, не для hot path)
    Value getVar(const std::string& pouName,
                 const std::string& varName) const;

    // ── Доступ к загруженным POU ──────────────────────────────

    // Возвращает список всех загруженных POU (для IOMapper и отладки)
    std::vector<const POUBytecode*> getPOUs() const {
        std::vector<const POUBytecode*> result;
        result.reserve(pous_.size());
        for (auto& kv : pous_)
            result.push_back(&kv.second);
        return result;
    }

    // Поиск конкретного POU по имени
    const POUBytecode* findPOU(const std::string& name) const {
        auto it = pous_.find(name);
        return (it != pous_.end()) ? &it->second : nullptr;
    }

private:
    std::unordered_map<std::string, POUBytecode>    pous_;
    std::unordered_map<std::string, ExecutionFrame> frameCache_;
    IOReadFn  ioRead_;
    IOWriteFn ioWrite_;

    void     initFrame   (ExecutionFrame& frame,
                          const POUBytecode& pou);
    void     execute     (ExecutionFrame& frame,
                          const std::vector<uint8_t>& code);
    Value    fetchOperand(ExecutionFrame& frame,
                          const std::vector<uint8_t>& code);
    uint16_t fetchVarIdx (ExecutionFrame& frame,
                          const std::vector<uint8_t>& code);
    uint16_t fetchU16    (ExecutionFrame& frame,
                          const std::vector<uint8_t>& code);
    uint32_t fetchU32    (ExecutionFrame& frame,
                          const std::vector<uint8_t>& code);
    void callFunctionBlock(ExecutionFrame& callerFrame,
                           uint16_t fbIdx);
};