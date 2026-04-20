// parser.hpp
#pragma once
#include "lexer.hpp"
#include "ast.hpp"
#include <vector>
#include <stdexcept>

class Parser {
public:
    explicit Parser(std::vector<Token> tokens)
        : tokens_(std::move(tokens)), pos_(0) {}

    std::vector<POU> parse();

private:
    std::vector<Token> tokens_;
    size_t             pos_;

    Token& peek();
    Token& consume();
    Token& expect(TokenType type);
    bool   check(TokenType type) const;
    bool   match(TokenType type);

    POU           parsePOU();
    VarDecl       parseVarDecl(VarDecl::Kind kind);
    ILInstruction parseInstruction();
    Operand       parseOperand(OpCodeAST context);
    IECType       parseType();

    [[noreturn]] void error(const std::string& msg);
};
