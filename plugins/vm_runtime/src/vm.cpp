// vm.cpp
#include "vm.hpp"
#include <stdexcept>
#include <cstring>
#include <iostream>

void VirtualMachine::loadPOU(const POUBytecode& pou) {  
    pous_[pou.name] = pou;
}

void VirtualMachine::setIOCallbacks(IOReadFn reader, IOWriteFn writer) {
    ioRead_  = std::move(reader);
    ioWrite_ = std::move(writer);
}

Value VirtualMachine::getVar(const std::string& pouName,
                              const std::string& varName) const
{
    auto pit = pous_.find(pouName);
    if (pit == pous_.end()) return Value{};
    auto vit = pit->second.varMap.find(varName);
    if (vit == pit->second.varMap.end()) return Value{};
    auto fit = frameCache_.find(pouName);
    if (fit == frameCache_.end()) return Value{};
    return fit->second.vars[vit->second];
}

void VirtualMachine::executeCycle(const std::string& programName) {
    auto it = pous_.find(programName);
    if (it == pous_.end())
        throw std::runtime_error("POU not found: " + programName);

    ExecutionFrame frame;
    initFrame(frame, it->second);
    execute(frame, it->second.code);
    frameCache_[programName] = frame;
}

void VirtualMachine::initFrame(ExecutionFrame& frame,
                                const POUBytecode& pou)
{
    auto it = frameCache_.find(pou.name);
    if (it != frameCache_.end()) {
        frame = it->second;
        frame.pc = 0;
        return;
    }
    frame.varCount = pou.vars.size();
    uint16_t idx = 0;
    for (auto& v : pou.vars) {
        switch (v.type) {
            case IECType::BOOL:
                frame.vars[idx] = Value::fromBool(
                    std::get<bool>(v.initialValue));
                break;
            case IECType::INT:
                frame.vars[idx] = Value::fromInt(
                    std::get<int16_t>(v.initialValue));
                break;
            case IECType::DINT:
                frame.vars[idx] = Value::fromDInt(
                    std::get<int32_t>(v.initialValue));
                break;
            case IECType::REAL:
                frame.vars[idx] = Value::fromReal(
                    std::get<float>(v.initialValue));
                break;
            default:
                frame.vars[idx] = Value::fromDInt(0);
        }
        ++idx;
    }
}

void VirtualMachine::execute(ExecutionFrame& frame,
                              const std::vector<uint8_t>& code)
{
    while (frame.pc < static_cast<uint16_t>(code.size())) {
        auto op = static_cast<OpCode>(code[frame.pc]);
        ++frame.pc;

        switch (op) {
            case OpCode::NOP:  break;
            case OpCode::HALT: return;
            case OpCode::RET:  return;

            case OpCode::LD:
                frame.cr = fetchOperand(frame, code);
                break;
            case OpCode::LDN:
                frame.cr = fetchOperand(frame, code).logicNot();
                break;
            case OpCode::ST:
                frame.var(fetchVarIdx(frame, code)) = frame.cr;
                break;
            case OpCode::STN:
                frame.var(fetchVarIdx(frame, code)) = frame.cr.logicNot();
                break;

            case OpCode::ADD:
                frame.cr = frame.cr + fetchOperand(frame, code);
                break;
            case OpCode::SUB:
                frame.cr = frame.cr - fetchOperand(frame, code);
                break;
            case OpCode::MUL:
                frame.cr = frame.cr * fetchOperand(frame, code);
                break;
            case OpCode::DIV:
                frame.cr = frame.cr / fetchOperand(frame, code);
                break;
            case OpCode::MOD:
                frame.cr = frame.cr % fetchOperand(frame, code);
                break;

            case OpCode::AND:
                frame.cr = frame.cr.logicAnd(fetchOperand(frame, code));
                break;
            case OpCode::ANDN:
                frame.cr = frame.cr.logicAnd(
                    fetchOperand(frame, code).logicNot());
                break;
            case OpCode::OR:
                frame.cr = frame.cr.logicOr(fetchOperand(frame, code));
                break;
            case OpCode::ORN:
                frame.cr = frame.cr.logicOr(
                    fetchOperand(frame, code).logicNot());
                break;
            case OpCode::XOR:
                frame.cr = frame.cr.logicXor(fetchOperand(frame, code));
                break;
            case OpCode::XORN:
                frame.cr = frame.cr.logicXor(
                    fetchOperand(frame, code).logicNot());
                break;
            case OpCode::NOT:
                frame.cr = frame.cr.logicNot();
                break;

            case OpCode::GT:
                frame.cr = Value::fromBool(
                    frame.cr > fetchOperand(frame, code));
                break;
            case OpCode::GE:
                frame.cr = Value::fromBool(
                    frame.cr >= fetchOperand(frame, code));
                break;
            case OpCode::LT:
                frame.cr = Value::fromBool(
                    frame.cr < fetchOperand(frame, code));
                break;
            case OpCode::LE:
                frame.cr = Value::fromBool(
                    frame.cr <= fetchOperand(frame, code));
                break;
            case OpCode::EQ:
                frame.cr = Value::fromBool(
                    frame.cr == fetchOperand(frame, code));
                break;
            case OpCode::NE:
                frame.cr = Value::fromBool(
                    frame.cr != fetchOperand(frame, code));
                break;

            case OpCode::PUSH_CR: frame.pushCR(); break;
            case OpCode::POP_CR:  frame.popCR();  break;

            case OpCode::JMP:
                frame.pc = fetchU16(frame, code);
                break;
            case OpCode::JMPC: {
                uint16_t addr = fetchU16(frame, code);
                if (frame.cr.asBool()) frame.pc = addr;
                break;
            }
            case OpCode::JMPCN: {
                uint16_t addr = fetchU16(frame, code);
                if (!frame.cr.asBool()) frame.pc = addr;
                break;
            }

            case OpCode::CAL:
            case OpCode::CALC:
            case OpCode::CALCN: {
                uint16_t fbIdx = fetchU16(frame, code);
                bool doCall = true;
                if (op == OpCode::CALC)  doCall =  frame.cr.asBool();
                if (op == OpCode::CALCN) doCall = !frame.cr.asBool();
                if (doCall) callFunctionBlock(frame, fbIdx);
                break;
            }

            default:
                throw std::runtime_error(
                    "Unknown opcode: 0x" +
                    std::to_string(static_cast<int>(op)));
        }
    }
}

Value VirtualMachine::fetchOperand(ExecutionFrame& frame,
                                    const std::vector<uint8_t>& code)
{
    auto tag = static_cast<OperandTag>(code[frame.pc++]);

    if (isVarTag(tag)) {
        // [VAR_* tag] уже прочитан, читаем 2 байта индекса
        uint16_t idx = fetchU16(frame, code);
        return frame.var(idx);
    }

    if (isConstTag(tag)) {
        switch (tag) {
            case OperandTag::CONST_BOOL:
                return Value::fromBool(code[frame.pc++] != 0);

            case OperandTag::CONST_INT: {
                int16_t v = static_cast<int16_t>(fetchU16(frame, code));
                return Value::fromInt(v);
            }

            case OperandTag::CONST_DINT: {
                int32_t v = static_cast<int32_t>(fetchU32(frame, code));
                return Value::fromDInt(v);
            }

            case OperandTag::CONST_REAL: {
                uint32_t bits = fetchU32(frame, code);
                float v;
                std::memcpy(&v, &bits, 4);
                return Value::fromReal(v);
            }

            default:
                break;
        }
    }

    throw std::runtime_error(
        "Invalid operand tag: 0x" +
        std::to_string(static_cast<int>(tag)));
}

// fetchVarIdx используется только для ST/STN — читает тег и индекс
uint16_t VirtualMachine::fetchVarIdx(ExecutionFrame& frame,
                                      const std::vector<uint8_t>& code)
{
    ++frame.pc; // пропустить тег (для ST всегда VAR_*)
    return fetchU16(frame, code);
}

uint16_t VirtualMachine::fetchU16(ExecutionFrame& frame,
                                   const std::vector<uint8_t>& code)
{
    uint16_t lo = code[frame.pc++];
    uint16_t hi = code[frame.pc++];
    return static_cast<uint16_t>(lo | (hi << 8));
}

uint32_t VirtualMachine::fetchU32(ExecutionFrame& frame,
                                   const std::vector<uint8_t>& code)
{
    uint32_t v  =  code[frame.pc++];
    v |= static_cast<uint32_t>(code[frame.pc++]) << 8;
    v |= static_cast<uint32_t>(code[frame.pc++]) << 16;
    v |= static_cast<uint32_t>(code[frame.pc++]) << 24;
    return v;
}

void VirtualMachine::callFunctionBlock(ExecutionFrame& callerFrame,
                                        uint16_t fbIdx)
{
    (void)callerFrame;
    (void)fbIdx;
    // TODO: реализация вызова ФБ
}
