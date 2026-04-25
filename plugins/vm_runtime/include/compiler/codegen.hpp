// codegen.hpp
#pragma once
#include "ast.hpp"
#include "opcodes.hpp"
#include <vector>
#include <unordered_map>

struct POUBytecode {
    std::string              name;
    std::vector<uint8_t>     code;
    std::unordered_map<std::string, uint16_t> varMap;
    std::vector<VarDecl>     vars;
};

struct PlcBytecodeFile {
    std::vector<POUBytecode> pous;
    std::vector<std::string> symbolTable;
};

class CodeGenerator {
public:
    POUBytecode generate(const POU& pou);

private:
    size_t instrSize(const ILInstruction& instr);
    size_t operandSize(const Operand& op);

    void emitInstruction(
        const ILInstruction& instr,
        POUBytecode& out,
        const std::unordered_map<std::string, uint16_t>& labelAddr,
        std::vector<std::pair<size_t, std::string>>& patchList);

	void emitOperand(const Operand& operand,
					const POUBytecode& pou,
					std::vector<uint8_t>& code);
};
