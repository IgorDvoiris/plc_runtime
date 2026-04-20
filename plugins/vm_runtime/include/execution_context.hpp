// execution_context.hpp
#pragma once
#include "value.hpp"
#include <array>
#include <stdexcept>

constexpr size_t MAX_VARS       = 256;
constexpr size_t CR_STACK_DEPTH = 32;

struct ExecutionFrame {
    Value cr;
    std::array<Value, CR_STACK_DEPTH> crStack;
    size_t   crStackTop = 0;
    std::array<Value, MAX_VARS> vars;
    size_t   varCount = 0;
    uint16_t pc = 0;

    void pushCR() {
        if (crStackTop >= CR_STACK_DEPTH)
            throw std::runtime_error("CR stack overflow");
        crStack[crStackTop++] = cr;
    }

    void popCR() {
        if (crStackTop == 0)
            throw std::runtime_error("CR stack underflow");
        cr = crStack[--crStackTop];
    }

    Value& var(uint16_t idx) {
        if (idx >= MAX_VARS)
            throw std::runtime_error("Variable index out of range");
        return vars[idx];
    }
};
