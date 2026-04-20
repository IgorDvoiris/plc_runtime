// lexer.cpp
#include "lexer.hpp"

const std::unordered_map<std::string, TokenType>& Lexer::keywords() {
    static std::unordered_map<std::string, TokenType> kw = {
        {"LD",    TokenType::LD},
        {"LDN",   TokenType::LDN},
        {"ST",    TokenType::ST},
        {"STN",   TokenType::STN},
        {"ADD",   TokenType::ADD},
        {"SUB",   TokenType::SUB},
        {"MUL",   TokenType::MUL},
        {"DIV",   TokenType::DIV},
        {"MOD",   TokenType::MOD},
        {"AND",   TokenType::AND},
        {"ANDN",  TokenType::ANDN},
        {"OR",    TokenType::OR},
        {"ORN",   TokenType::ORN},
        {"XOR",   TokenType::XOR},
        {"XORN",  TokenType::XORN},
        {"NOT",   TokenType::NOT},
        {"GT",    TokenType::GT},
        {"GE",    TokenType::GE},
        {"LT",    TokenType::LT},
        {"LE",    TokenType::LE},
        {"EQ",    TokenType::EQ},
        {"NE",    TokenType::NE},
        {"JMP",   TokenType::JMP},
        {"JMPC",  TokenType::JMPC},
        {"JMPCN", TokenType::JMPCN},
        {"CAL",   TokenType::CAL},
        {"CALC",  TokenType::CALC},
        {"CALCN", TokenType::CALCN},
        {"RET",   TokenType::RET},
        {"PROGRAM",              TokenType::KW_PROGRAM},
        {"END_PROGRAM",          TokenType::KW_END_PROGRAM},
        {"FUNCTION",             TokenType::KW_FUNCTION},
        {"END_FUNCTION",         TokenType::KW_END_FUNCTION},
        {"FUNCTION_BLOCK",       TokenType::KW_FUNCTION_BLOCK},
        {"END_FUNCTION_BLOCK",   TokenType::KW_END_FUNCTION_BLOCK},
        {"VAR",                  TokenType::KW_VAR},
        {"VAR_INPUT",            TokenType::KW_VAR_INPUT},
        {"VAR_OUTPUT",           TokenType::KW_VAR_OUTPUT},
        {"VAR_IN_OUT",           TokenType::KW_VAR_IN_OUT},
        {"END_VAR",              TokenType::KW_END_VAR},
        {"BOOL",   TokenType::TYPE_BOOL},
        {"INT",    TokenType::TYPE_INT},
        {"DINT",   TokenType::TYPE_DINT},
        {"REAL",   TokenType::TYPE_REAL},
        {"TIME",   TokenType::TYPE_TIME},
        {"TRUE",   TokenType::BOOL_LIT},
        {"FALSE",  TokenType::BOOL_LIT},
    };
    return kw;
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (pos_ < src_.size()) {
        skipWhitespaceAndComments();
        if (pos_ >= src_.size()) break;
        tokens.push_back(nextToken());
    }
    tokens.push_back({TokenType::END_OF_FILE, "", line_, col_});
    return tokens;
}

void Lexer::skipWhitespaceAndComments() {
    while (pos_ < src_.size()) {
        if (src_[pos_] == '\n') {
            ++line_; col_ = 1; ++pos_;
        } else if (std::isspace(static_cast<unsigned char>(src_[pos_]))) {
            ++col_; ++pos_;
        } else if (pos_ + 1 < src_.size() &&
                   src_[pos_] == '(' && src_[pos_+1] == '*') {
            pos_ += 2;
            while (pos_ + 1 < src_.size() &&
                   !(src_[pos_] == '*' && src_[pos_+1] == ')')) {
                if (src_[pos_] == '\n') { ++line_; col_ = 1; }
                ++pos_;
            }
            pos_ += 2;
        } else if (pos_ + 1 < src_.size() &&
                   src_[pos_] == '/' && src_[pos_+1] == '/') {
            while (pos_ < src_.size() && src_[pos_] != '\n') ++pos_;
        } else {
            break;
        }
    }
}

Token Lexer::nextToken() {
    int startLine = line_, startCol = col_;
    char c = src_[pos_];

    if (std::isdigit(static_cast<unsigned char>(c)) ||
        (c == '-' && pos_+1 < src_.size() &&
         std::isdigit(static_cast<unsigned char>(src_[pos_+1])))) {
        return readNumber(startLine, startCol);
    }
    if (c == '\'') return readString(startLine, startCol);
    if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
        return readIdentOrKeyword(startLine, startCol);
    }

    ++pos_; ++col_;
    switch (c) {
        case ':':
            if (pos_ < src_.size() && src_[pos_] == '=') {
                ++pos_; ++col_;
                return {TokenType::ASSIGN, ":=", startLine, startCol};
            }
            return {TokenType::COLON, ":", startLine, startCol};
        case ';': return {TokenType::SEMICOLON, ";", startLine, startCol};
        case ',': return {TokenType::COMMA,     ",", startLine, startCol};
        case '(': return {TokenType::LPAREN,    "(", startLine, startCol};
        case ')': return {TokenType::RPAREN,    ")", startLine, startCol};
        default:
            return {TokenType::UNKNOWN, std::string(1, c), startLine, startCol};
    }
}

Token Lexer::readNumber(int l, int c) {
    size_t start = pos_;
    bool isReal = false;
    if (src_[pos_] == '-') ++pos_;
    while (pos_ < src_.size() &&
           std::isdigit(static_cast<unsigned char>(src_[pos_]))) ++pos_;
    if (pos_ < src_.size() && src_[pos_] == '.') {
        isReal = true;
        ++pos_;
        while (pos_ < src_.size() &&
               std::isdigit(static_cast<unsigned char>(src_[pos_]))) ++pos_;
    }
    std::string val = src_.substr(start, pos_ - start);
    col_ += static_cast<int>(pos_ - start);
    return {isReal ? TokenType::REAL_LIT : TokenType::INT_LIT, val, l, c};
}

Token Lexer::readString(int l, int c) {
    ++pos_;
    size_t start = pos_;
    while (pos_ < src_.size() && src_[pos_] != '\'') ++pos_;
    std::string val = src_.substr(start, pos_ - start);
    ++pos_;
    return {TokenType::STRING_LIT, val, l, c};
}

Token Lexer::readIdentOrKeyword(int l, int c) {
    size_t start = pos_;
    while (pos_ < src_.size() &&
           (std::isalnum(static_cast<unsigned char>(src_[pos_])) ||
            src_[pos_] == '_')) ++pos_;
    std::string word = src_.substr(start, pos_ - start);
    col_ += static_cast<int>(pos_ - start);

    if (pos_ < src_.size() && src_[pos_] == ':' &&
        !(pos_+1 < src_.size() && src_[pos_+1] == '=')) {
        ++pos_; ++col_;
        return {TokenType::LABEL, word, l, c};
    }

    auto& kw = keywords();
    auto it = kw.find(word);
    if (it != kw.end()) return {it->second, word, l, c};
    return {TokenType::IDENTIFIER, word, l, c};
}
