// opcodes.hpp
#pragma once
#include <cstdint>

enum class OpCode : uint8_t {
    LD      = 0x01,
    LDN     = 0x02,
    ST      = 0x03,
    STN     = 0x04,
    ADD     = 0x10,
    SUB     = 0x11,
    MUL     = 0x12,
    DIV     = 0x13,
    MOD     = 0x14,
    AND     = 0x20,
    ANDN    = 0x21,
    OR      = 0x22,
    ORN     = 0x23,
    XOR     = 0x24,
    XORN    = 0x25,
    NOT     = 0x26,
    GT      = 0x30,
    GE      = 0x31,
    LT      = 0x32,
    LE      = 0x33,
    EQ      = 0x34,
    NE      = 0x35,
    PUSH_CR = 0x40,
    POP_CR  = 0x41,
    JMP     = 0x50,
    JMPC    = 0x51,
    JMPCN   = 0x52,
    CAL     = 0x53,
    CALC    = 0x54,
    CALCN   = 0x55,
    RET     = 0x56,
    NOP     = 0xFE,
    HALT    = 0xFF,
};

// Байт-дискриминатор операнда, следующий сразу после opcode.
// Значения 0x01-0x0F — переменная указанного типа (далее 2 байта индекса).
// Значения 0x11-0x1F — инлайн-константа указанного типа (далее N байт значения).
enum class OperandTag : uint8_t {
    // Переменная: [tag:1][varIdx:2]
    VAR_BOOL   = 0x01,
    VAR_INT    = 0x02,
    VAR_DINT   = 0x03,
    VAR_REAL   = 0x04,
    VAR_TIME   = 0x05,

    // Инлайн-константа: [tag:1][value:N]
    CONST_BOOL = 0x11,  // далее 1 байт
    CONST_INT  = 0x12,  // далее 2 байта (little-endian int16)
    CONST_DINT = 0x13,  // далее 4 байта (little-endian int32)
    CONST_REAL = 0x14,  // далее 4 байта (IEEE 754 float)
};

// Вспомогательные предикаты
inline bool isVarTag(OperandTag t) {
    return static_cast<uint8_t>(t) >= 0x01 &&
           static_cast<uint8_t>(t) <= 0x0F;
}

inline bool isConstTag(OperandTag t) {
    return static_cast<uint8_t>(t) >= 0x11 &&
           static_cast<uint8_t>(t) <= 0x1F;
}
