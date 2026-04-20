// type_checker.hpp
#pragma once
#include "ast.hpp"
#include <vector>
#include <string>
#include <stdexcept>

class TypeChecker {
public:
    void check(const std::vector<POU>& pous);

private:
    void checkPOU(const POU& pou);
    void checkInstruction(const ILInstruction& instr,
                          const POU& pou);

    const VarDecl* findVar(const std::string& name,
                           const POU& pou) const;
};
