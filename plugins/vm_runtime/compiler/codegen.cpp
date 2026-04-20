// codegen.cpp
#include "codegen.hpp"
#include <cstring>
#include <stdexcept>

POUBytecode CodeGenerator::generate(const POU& pou) {
    POUBytecode result;
    result.name = pou.name;
    result.vars = pou.variables;

    uint16_t idx = 0;
    for (auto& v : pou.variables)
        result.varMap[v.name] = idx++;

    // Первый проход: адреса меток
    std::unordered_map<std::string, uint16_t> labelAddr;
    size_t offset = 0;
    for (auto& instr : pou.body) {
        if (!instr.label.empty())
            labelAddr[instr.label] = static_cast<uint16_t>(offset);
        offset += instrSize(instr);
    }

    // Второй проход: генерация
    std::vector<std::pair<size_t, std::string>> patchList;
    for (auto& instr : pou.body)
        emitInstruction(instr, result, labelAddr, patchList);

    // Патч адресов переходов
    for (auto& kv : patchList) {
        auto it = labelAddr.find(kv.second);
        if (it == labelAddr.end())
            throw std::runtime_error("Undefined label: " + kv.second);
        uint16_t addr = it->second;
        result.code[kv.first]   =  addr & 0xFF;
        result.code[kv.first+1] = (addr >> 8) & 0xFF;
    }

    result.code.push_back(static_cast<uint8_t>(OpCode::RET));
    return result;
}

// ── Размер инструкции в байтах ────────────────────────────────────────────
size_t CodeGenerator::instrSize(const ILInstruction& instr) {
    switch (instr.opcode) {
        case OpCodeAST::NOT:
        case OpCodeAST::RET:
            return 1;                             // только opcode

        case OpCodeAST::JMP:
        case OpCodeAST::JMPC:
        case OpCodeAST::JMPCN:
        case OpCodeAST::CAL:
        case OpCodeAST::CALC:
        case OpCodeAST::CALCN:
            return 3;                             // opcode + addr/idx (2)

        default:
            return 1 + operandSize(instr.operand); // opcode + операнд
    }
}

// Размер операнда в байтах
size_t CodeGenerator::operandSize(const Operand& op) {
    switch (op.kind) {
        case Operand::Kind::VARIABLE:   return 3; // tag(1) + varIdx(2)
        case Operand::Kind::CONST_BOOL: return 2; // tag(1) + value(1)
        case Operand::Kind::CONST_INT:  return 3; // tag(1) + value(2)
        case Operand::Kind::CONST_DINT: return 5; // tag(1) + value(4)
        case Operand::Kind::CONST_REAL: return 5; // tag(1) + value(4)
        default:                        return 0;
    }
}

// ── Эмиссия инструкций ────────────────────────────────────────────────────
void CodeGenerator::emitInstruction(
    const ILInstruction& instr,
    POUBytecode& out,
    const std::unordered_map<std::string, uint16_t>& labelAddr,
    std::vector<std::pair<size_t, std::string>>& patchList)
{
    auto& code = out.code;
    auto emitOp = [&](OpCode op) {
        // opcode + инлайн-операнд (переменная или константа)
        code.push_back(static_cast<uint8_t>(op));
        emitOperand(instr.operand, out, code);
    };

    switch (instr.opcode) {
        case OpCodeAST::LD:   emitOp(OpCode::LD);   break;
        case OpCodeAST::LDN:  emitOp(OpCode::LDN);  break;
        case OpCodeAST::ST:   emitOp(OpCode::ST);   break;
        case OpCodeAST::STN:  emitOp(OpCode::STN);  break;
        case OpCodeAST::ADD:  emitOp(OpCode::ADD);  break;
        case OpCodeAST::SUB:  emitOp(OpCode::SUB);  break;
        case OpCodeAST::MUL:  emitOp(OpCode::MUL);  break;
        case OpCodeAST::DIV:  emitOp(OpCode::DIV);  break;
        case OpCodeAST::MOD:  emitOp(OpCode::MOD);  break;
        case OpCodeAST::AND:  emitOp(OpCode::AND);  break;
        case OpCodeAST::ANDN: emitOp(OpCode::ANDN); break;
        case OpCodeAST::OR:   emitOp(OpCode::OR);   break;
        case OpCodeAST::ORN:  emitOp(OpCode::ORN);  break;
        case OpCodeAST::XOR:  emitOp(OpCode::XOR);  break;
        case OpCodeAST::XORN: emitOp(OpCode::XORN); break;
        case OpCodeAST::GT:   emitOp(OpCode::GT);   break;
        case OpCodeAST::GE:   emitOp(OpCode::GE);   break;
        case OpCodeAST::LT:   emitOp(OpCode::LT);   break;
        case OpCodeAST::LE:   emitOp(OpCode::LE);   break;
        case OpCodeAST::EQ:   emitOp(OpCode::EQ);   break;
        case OpCodeAST::NE:   emitOp(OpCode::NE);   break;

        case OpCodeAST::NOT:
            code.push_back(static_cast<uint8_t>(OpCode::NOT));
            break;

        case OpCodeAST::RET:
            code.push_back(static_cast<uint8_t>(OpCode::RET));
            break;

        case OpCodeAST::JMP:
        case OpCodeAST::JMPC:
        case OpCodeAST::JMPCN: {
            OpCode op = (instr.opcode == OpCodeAST::JMP)   ? OpCode::JMP   :
                        (instr.opcode == OpCodeAST::JMPC)  ? OpCode::JMPC  :
                                                              OpCode::JMPCN;
            code.push_back(static_cast<uint8_t>(op));
            patchList.push_back({code.size(), instr.operand.strVal});
            code.push_back(0x00); // placeholder lo
            code.push_back(0x00); // placeholder hi
            break;
        }

        case OpCodeAST::CAL:
        case OpCodeAST::CALC:
        case OpCodeAST::CALCN: {
            OpCode op = (instr.opcode == OpCodeAST::CAL)   ? OpCode::CAL   :
                        (instr.opcode == OpCodeAST::CALC)  ? OpCode::CALC  :
                                                              OpCode::CALCN;
            code.push_back(static_cast<uint8_t>(op));
            auto it = out.varMap.find(instr.operand.strVal);
            uint16_t fbIdx = (it != out.varMap.end()) ? it->second : 0xFFFF;
            code.push_back( fbIdx & 0xFF);
            code.push_back((fbIdx >> 8) & 0xFF);
            break;
        }

        default:
            code.push_back(static_cast<uint8_t>(OpCode::NOP));
            break;
    }

    (void)labelAddr;
}

// ── Эмиссия операнда (единый формат) ─────────────────────────────────────
void CodeGenerator::emitOperand(const Operand& operand,
                                 const POUBytecode& pou,
                                 std::vector<uint8_t>& code)
{
    switch (operand.kind) {

        case Operand::Kind::VARIABLE: {
            auto it = pou.varMap.find(operand.strVal);
            if (it == pou.varMap.end())
                throw std::runtime_error(
                    "Unknown variable: " + operand.strVal);
            uint16_t idx = it->second;

            // Определяем тип переменной → выбираем VAR_* тег
            OperandTag tag = OperandTag::VAR_DINT;
            for (auto& v : pou.vars) {
                if (v.name == operand.strVal) {
                    switch (v.type) {
                        case IECType::BOOL: tag = OperandTag::VAR_BOOL; break;
                        case IECType::INT:  tag = OperandTag::VAR_INT;  break;
                        case IECType::DINT: tag = OperandTag::VAR_DINT; break;
                        case IECType::REAL: tag = OperandTag::VAR_REAL; break;
                        case IECType::TIME: tag = OperandTag::VAR_TIME; break;
                        default: break;
                    }
                    break;
                }
            }
            code.push_back(static_cast<uint8_t>(tag)); // тег
            code.push_back( idx & 0xFF);               // индекс lo
            code.push_back((idx >> 8) & 0xFF);         // индекс hi
            break;
        }

        case Operand::Kind::CONST_BOOL:
            code.push_back(static_cast<uint8_t>(OperandTag::CONST_BOOL));
            code.push_back(std::get<bool>(operand.constVal) ? 1u : 0u);
            break;

        case Operand::Kind::CONST_INT: {
            code.push_back(static_cast<uint8_t>(OperandTag::CONST_INT));
            int16_t v = std::get<int16_t>(operand.constVal);
            code.push_back(static_cast<uint8_t>( v & 0xFF));
            code.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
            break;
        }

        case Operand::Kind::CONST_DINT: {
            code.push_back(static_cast<uint8_t>(OperandTag::CONST_DINT));
            int32_t v = std::get<int32_t>(operand.constVal);
            code.push_back(static_cast<uint8_t>((v >>  0) & 0xFF));
            code.push_back(static_cast<uint8_t>((v >>  8) & 0xFF));
            code.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
            code.push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
            break;
        }

        case Operand::Kind::CONST_REAL: {
            code.push_back(static_cast<uint8_t>(OperandTag::CONST_REAL));
            float v = std::get<float>(operand.constVal);
            uint32_t bits;
            std::memcpy(&bits, &v, 4);
            code.push_back(static_cast<uint8_t>((bits >>  0) & 0xFF));
            code.push_back(static_cast<uint8_t>((bits >>  8) & 0xFF));
            code.push_back(static_cast<uint8_t>((bits >> 16) & 0xFF));
            code.push_back(static_cast<uint8_t>((bits >> 24) & 0xFF));
            break;
        }

        default:
            // LABEL_REF, FB_INSTANCE, NONE — операнд кодируется отдельно
            break;
    }
}
