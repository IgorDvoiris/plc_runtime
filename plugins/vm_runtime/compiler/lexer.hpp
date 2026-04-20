// lexer.hpp
#pragma once
#include <string>
#include <vector>
#include <stdexcept>
#include <cctype>
#include <unordered_map>

enum class TokenType {
    LD, LDN, ST, STN,
    ADD, SUB, MUL, DIV, MOD,
    AND, ANDN, OR, ORN, XOR, XORN, NOT,
    GT, GE, LT, LE, EQ, NE,
    JMP, JMPC, JMPCN,
    CAL, CALC, CALCN,
    RET,
    BOOL_LIT,
    INT_LIT,
    REAL_LIT,
    STRING_LIT,
    IDENTIFIER,
    LABEL,
    KW_PROGRAM, KW_END_PROGRAM,
    KW_FUNCTION, KW_END_FUNCTION,
    KW_FUNCTION_BLOCK, KW_END_FUNCTION_BLOCK,
    KW_VAR, KW_VAR_INPUT, KW_VAR_OUTPUT,
    KW_VAR_IN_OUT, KW_END_VAR,
    KW_AT,          // директива AT
    TYPE_BOOL, TYPE_INT, TYPE_DINT, TYPE_REAL, TYPE_TIME,
    DIRECT_ADDR,    // %IX0.0 / %QW1 / %MD0
    COLON, SEMICOLON, ASSIGN, COMMA, LPAREN, RPAREN,
    END_OF_FILE,
    UNKNOWN
};

struct Token {
    TokenType   type;
    std::string value;
    int         line;
    int         col;
};

class Lexer {
public:
    explicit Lexer(std::string source)
        : src_(std::move(source)), pos_(0), line_(1), col_(1) {}

    std::vector<Token> tokenize();

private:
    std::string src_;
    size_t      pos_;
    int         line_, col_;

    static const std::unordered_map<std::string, TokenType>& keywords();

    void  skipWhitespaceAndComments();
    Token nextToken();
    Token readNumber       (int l, int c);
    Token readString       (int l, int c);
    Token readIdentOrKeyword(int l, int c);
    Token readDirectAddress(int l, int c); // новый метод
};