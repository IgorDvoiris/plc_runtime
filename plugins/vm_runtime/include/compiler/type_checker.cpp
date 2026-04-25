// type_checker.cpp
#include "type_checker.hpp"

void TypeChecker::check(const std::vector<POU>& pous) {
    for (auto& pou : pous) checkPOU(pou);
}

void TypeChecker::checkPOU(const POU& pou) {
    for (auto& instr : pou.body) checkInstruction(instr, pou);
}

void TypeChecker::checkInstruction(const ILInstruction& instr,
                                   const POU& pou)
{
    const Operand& op = instr.operand;

    switch (op.kind) {

        case Operand::Kind::NONE:
			break;

        case Operand::Kind::VARIABLE: {
            const VarDecl* v = findVar(op.strVal, pou);
            if (!v)
                throw std::runtime_error(
                    "Undefined variable '" + op.strVal +
                    "' at line " + std::to_string(instr.sourceLine));
            break;
        }

        case Operand::Kind::LABEL_REF:
			if (op.strVal.empty())
                throw std::runtime_error(
                    "Empty label reference at line " +
                    std::to_string(instr.sourceLine));
            break;

        case Operand::Kind::FB_INSTANCE: {
			if (op.strVal.empty())
                throw std::runtime_error(
                    "Empty FB instance name at line " +
                    std::to_string(instr.sourceLine));

            const VarDecl* v = findVar(op.strVal, pou);
            if (!v)
                throw std::runtime_error(
                    "Undefined FB instance '" + op.strVal +
                    "' at line " + std::to_string(instr.sourceLine));
            break;
        }

        case Operand::Kind::CONST_BOOL:
        case Operand::Kind::CONST_INT:
        case Operand::Kind::CONST_DINT:
        case Operand::Kind::CONST_REAL:
            break;
    }
}

const VarDecl* TypeChecker::findVar(const std::string& name,
                                    const POU& pou) const
{
    for (auto& v : pou.variables)
        if (v.name == name) return &v;
    return nullptr;
}
