// ast.hpp
#pragma once
#include <string>
#include <vector>
#include <memory>
#include <variant>
#include "opcodes.hpp"

enum class IECType { BOOL, INT, DINT, REAL, TIME, STRING, UNKNOWN };

using IECValue = std::variant<bool, int16_t, int32_t, float, std::string>;

struct VarDecl {
    std::string name;
    IECType     type;
    IECValue    initialValue;
    enum class Kind { LOCAL, INPUT, OUTPUT, IN_OUT } kind;
    uint16_t    index;
};

struct Operand {
    enum class Kind {
        UNKNOWN,
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
    IECType     type;
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
    bool        negated;
    bool        conditional;
    Operand     operand;
    int         sourceLine;
};

struct POU {
    enum class Type { PROGRAM, FUNCTION, FUNCTION_BLOCK } type;
    std::string name;
    std::vector<VarDecl>       variables;
    std::vector<ILInstruction> body;
};

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
