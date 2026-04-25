// ast.hpp
#pragma once
#include <string>
#include <vector>
#include <memory>
#include <variant>
#include "opcodes.hpp"

enum class IECType { BOOL, INT, DINT, REAL, TIME, STRING, UNKNOWN };

using IECValue = std::variant<bool, int16_t, int32_t, float, std::string>;

// ── Прямой адрес IO (%IX0.0, %QW1, %MD0 и т.д.) ─────────────────────────

enum class DirectAddressPrefix : uint8_t {
    INPUT  = 0,   // %I
    OUTPUT = 1,   // %Q
    MEMORY = 2,   // %M
};

enum class DirectAddressSize : uint8_t {
    BIT   = 0,   // X
    BYTE  = 1,   // B
    WORD  = 2,   // W
    DWORD = 3,   // D
};

struct DirectAddress {
    DirectAddressPrefix prefix;   // I / Q / M
    DirectAddressSize   size;     // X / B / W / D
    uint16_t            byteAddr; // адрес байта
    uint8_t             bitAddr;  // адрес бита (только для X)
    bool                hasBit;   // true если есть .bitAddr

    // Сериализация в строку для отладки: %IX0.0
    std::string toString() const {
        std::string s = "%";
        switch (prefix) {
            case DirectAddressPrefix::INPUT:  s += 'I'; break;
            case DirectAddressPrefix::OUTPUT: s += 'Q'; break;
            case DirectAddressPrefix::MEMORY: s += 'M'; break;
        }
        switch (size) {
            case DirectAddressSize::BIT:   s += 'X'; break;
            case DirectAddressSize::BYTE:  s += 'B'; break;
            case DirectAddressSize::WORD:  s += 'W'; break;
            case DirectAddressSize::DWORD: s += 'D'; break;
        }
        s += std::to_string(byteAddr);
        if (hasBit) {
            s += '.';
            s += std::to_string(bitAddr);
        }
        return s;
    }
};

struct VarDecl {
    std::string name;
    IECType     type;
    IECValue    initialValue;

    enum class Kind { LOCAL, INPUT, OUTPUT, IN_OUT } kind;
    uint16_t    index;  // индекс в таблице переменных VM

    // Прямой адрес (если есть директива AT)
    bool          hasDirectAddress = false;
    DirectAddress directAddress;
};

// ── Остальные структуры AST (без изменений) ───────────────────────────────

struct Operand {
    enum class Kind {
        VARIABLE,
        CONST_BOOL,
        CONST_INT,
        CONST_DINT,
        CONST_REAL,
        LABEL_REF,
        FB_INSTANCE,
        NONE
    } kind = Kind::NONE;

    std::string strVal;
    IECValue    constVal;
    IECType     type = IECType::UNKNOWN;
};

enum class OpCodeAST {
    LD, LDN, ST, STN,
    ADD, SUB, MUL, DIV, MOD,
    AND, ANDN, OR, ORN, XOR, XORN, NOT,
    GT, GE, LT, LE, EQ, NE,
    JMP, JMPC, JMPCN,
    CAL, CALC, CALCN,
    RET
};

struct ILInstruction {
    std::string label;
    OpCodeAST   opcode;
    bool        negated     = false;
    bool        conditional = false;
    Operand     operand;
    int         sourceLine  = 0;
};

struct POU {
    enum class Type { PROGRAM, FUNCTION, FUNCTION_BLOCK } type;
    std::string                name;
    std::vector<VarDecl>       variables;
    std::vector<ILInstruction> body;
};

// ── Вспомогательные функции классификации опкодов ─────────────────────────

inline bool opcodeHasLabelOperand(OpCodeAST op) {
    return op == OpCodeAST::JMP  ||
           op == OpCodeAST::JMPC ||
           op == OpCodeAST::JMPCN;
}

inline bool opcodeHasFBOperand(OpCodeAST op) {
    return op == OpCodeAST::CAL  ||
           op == OpCodeAST::CALC ||
           op == OpCodeAST::CALCN;
}