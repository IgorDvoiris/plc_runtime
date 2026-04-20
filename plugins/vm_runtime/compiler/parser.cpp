// parser.cpp
#include "parser.hpp"
#include "direct_address_parser.hpp"
#include <stdexcept>
#include <map>

static const std::map<TokenType, OpCodeAST> tokenToOpcode = {
    {TokenType::LD,    OpCodeAST::LD},
    {TokenType::LDN,   OpCodeAST::LDN},
    {TokenType::ST,    OpCodeAST::ST},
    {TokenType::STN,   OpCodeAST::STN},
    {TokenType::ADD,   OpCodeAST::ADD},
    {TokenType::SUB,   OpCodeAST::SUB},
    {TokenType::MUL,   OpCodeAST::MUL},
    {TokenType::DIV,   OpCodeAST::DIV},
    {TokenType::MOD,   OpCodeAST::MOD},
    {TokenType::AND,   OpCodeAST::AND},
    {TokenType::ANDN,  OpCodeAST::ANDN},
    {TokenType::OR,    OpCodeAST::OR},
    {TokenType::ORN,   OpCodeAST::ORN},
    {TokenType::XOR,   OpCodeAST::XOR},
    {TokenType::XORN,  OpCodeAST::XORN},
    {TokenType::NOT,   OpCodeAST::NOT},
    {TokenType::GT,    OpCodeAST::GT},
    {TokenType::GE,    OpCodeAST::GE},
    {TokenType::LT,    OpCodeAST::LT},
    {TokenType::LE,    OpCodeAST::LE},
    {TokenType::EQ,    OpCodeAST::EQ},
    {TokenType::NE,    OpCodeAST::NE},
    {TokenType::JMP,   OpCodeAST::JMP},
    {TokenType::JMPC,  OpCodeAST::JMPC},
    {TokenType::JMPCN, OpCodeAST::JMPCN},
    {TokenType::CAL,   OpCodeAST::CAL},
    {TokenType::CALC,  OpCodeAST::CALC},
    {TokenType::CALCN, OpCodeAST::CALCN},
    {TokenType::RET,   OpCodeAST::RET},
};

Token& Parser::peek()                { return tokens_[pos_]; }
Token& Parser::consume()             { return tokens_[pos_++]; }
bool   Parser::check(TokenType t) const { return tokens_[pos_].type == t; }

bool Parser::match(TokenType t) {
    if (check(t)) { ++pos_; return true; }
    return false;
}

Token& Parser::expect(TokenType type) {
    if (!check(type))
        error("Expected token type " + std::to_string(static_cast<int>(type)) +
              " but got '" + peek().value + "' at line " +
              std::to_string(peek().line));
    return consume();
}

void Parser::error(const std::string& msg) {
    throw std::runtime_error("Parse error: " + msg);
}

std::vector<POU> Parser::parse() {
    std::vector<POU> pous;
    while (!check(TokenType::END_OF_FILE)) {
        pous.push_back(parsePOU());
    }
    return pous;
}

POU Parser::parsePOU() {
    POU pou;
    if (match(TokenType::KW_PROGRAM)) {
        pou.type = POU::Type::PROGRAM;
    } else if (match(TokenType::KW_FUNCTION_BLOCK)) {
        pou.type = POU::Type::FUNCTION_BLOCK;
    } else if (match(TokenType::KW_FUNCTION)) {
        pou.type = POU::Type::FUNCTION;
    } else {
        error("Expected PROGRAM, FUNCTION or FUNCTION_BLOCK");
    }

    pou.name = expect(TokenType::IDENTIFIER).value;

    // Секции переменных
    while (check(TokenType::KW_VAR)     ||
           check(TokenType::KW_VAR_INPUT)  ||
           check(TokenType::KW_VAR_OUTPUT) ||
           check(TokenType::KW_VAR_IN_OUT))
    {
        VarDecl::Kind kind = VarDecl::Kind::LOCAL;
        if      (match(TokenType::KW_VAR_INPUT))  kind = VarDecl::Kind::INPUT;
        else if (match(TokenType::KW_VAR_OUTPUT)) kind = VarDecl::Kind::OUTPUT;
        else if (match(TokenType::KW_VAR_IN_OUT)) kind = VarDecl::Kind::IN_OUT;
        else                                       match(TokenType::KW_VAR);

        while (!check(TokenType::KW_END_VAR) &&
               !check(TokenType::END_OF_FILE)) {
            pou.variables.push_back(parseVarDecl(kind));
        }
        expect(TokenType::KW_END_VAR);
    }

    // Назначаем индексы переменным
    uint16_t idx = 0;
    for (auto& v : pou.variables) v.index = idx++;

    // Тело программы
    while (!check(TokenType::KW_END_PROGRAM)      &&
           !check(TokenType::KW_END_FUNCTION)      &&
           !check(TokenType::KW_END_FUNCTION_BLOCK)&&
           !check(TokenType::END_OF_FILE))
    {
        pou.body.push_back(parseInstruction());
    }

    // Пропускаем закрывающее ключевое слово
    consume();
    return pou;
}


VarDecl Parser::parseVarDecl(VarDecl::Kind kind) {
    VarDecl v;
    v.kind             = kind;
    v.hasDirectAddress = false;
    v.name = expect(TokenType::IDENTIFIER).value;

    // ── Директива AT ──────────────────────────────────────────
    if (match(TokenType::KW_AT)) {
        if (!check(TokenType::DIRECT_ADDR))
            error("Expected direct address (e.g. %IX0.0) after AT"
                  ", got: '" + peek().value + "'");

        v.hasDirectAddress = true;
        v.directAddress    = DirectAddressParser::parse(peek().value);
        consume(); // потребляем DIRECT_ADDR
    }

    expect(TokenType::COLON);
    v.type = parseType();

    // Проверка совместимости адреса и типа
    if (v.hasDirectAddress)
        validateAddressType(v.directAddress, v.type, v.name);

    // Начальное значение
    if (match(TokenType::ASSIGN)) {
        switch (v.type) {
            case IECType::BOOL:
                v.initialValue = (peek().value == "TRUE");
                consume();
                break;
            case IECType::INT:
                v.initialValue = static_cast<int16_t>(
                    std::stoi(consume().value));
                break;
            case IECType::DINT:
                v.initialValue = static_cast<int32_t>(
                    std::stol(consume().value));
                break;
            case IECType::REAL:
                v.initialValue = std::stof(consume().value);
                break;
            default:
                v.initialValue = int32_t{0};
                consume();
        }
    } else {
        switch (v.type) {
            case IECType::BOOL:  v.initialValue = false;      break;
            case IECType::INT:   v.initialValue = int16_t{0}; break;
            case IECType::DINT:  v.initialValue = int32_t{0}; break;
            case IECType::REAL:  v.initialValue = 0.0f;       break;
            default:             v.initialValue = int32_t{0}; break;
        }
    }

    expect(TokenType::SEMICOLON);
    return v;
}

void Parser::validateAddressType(const DirectAddress& addr,
                                  IECType type,
                                  const std::string& name)
{
    bool ok = false;
    switch (addr.size) {
        case DirectAddressSize::BIT:
            ok = (type == IECType::BOOL);
            break;
        case DirectAddressSize::BYTE:
            ok = (type == IECType::INT || type == IECType::BOOL);
            break;
        case DirectAddressSize::WORD:
            ok = (type == IECType::INT || type == IECType::BOOL);
            break;
        case DirectAddressSize::DWORD:
            ok = (type == IECType::DINT || type == IECType::REAL);
            break;
    }
    if (!ok)
        error("AT variable '" + name +
              "': address size incompatible with declared type");
}

ILInstruction Parser::parseInstruction() {
    ILInstruction instr{};
    instr.negated     = false;
    instr.conditional = false;
    instr.sourceLine  = peek().line;

    // Метка уже разобрана лексером как LABEL-токен
    if (check(TokenType::LABEL)) {
        instr.label = consume().value;
    }

    if (check(TokenType::END_OF_FILE)) return instr;

    auto it = tokenToOpcode.find(peek().type);
    if (it == tokenToOpcode.end())
        error("Expected IL instruction at line " +
              std::to_string(peek().line) + ", got: '" + peek().value + "'");

    instr.opcode = it->second;
    consume();

    // Операнды для инструкций, которым они нужны
    switch (instr.opcode) {
        case OpCodeAST::NOT:
        case OpCodeAST::RET:
            break;
        default:
            instr.operand = parseOperand(instr.opcode);
    }

    return instr;
}

Operand Parser::parseOperand(OpCodeAST context) {
    Operand op{};
    Token& t = peek();

    if (opcodeHasLabelOperand(context)) {
        // Операнд перехода — всегда идентификатор без ':'
        // (метки-назначения не имеют ':' в позиции операнда)
        if (t.type != TokenType::IDENTIFIER &&
            t.type != TokenType::LABEL)          // на случай "JMP LOOP:"
            error("Expected label name after jump instruction, got: '" +
                  t.value + "'");
        op.kind   = Operand::Kind::LABEL_REF;
        op.strVal = t.value;
        op.type   = IECType::UNKNOWN;
        consume();
        return op;
    }

    if (opcodeHasFBOperand(context)) {
        if (t.type != TokenType::IDENTIFIER)
            error("Expected FB instance name, got: '" + t.value + "'");
        op.kind   = Operand::Kind::FB_INSTANCE;
        op.strVal = t.value;
        op.type   = IECType::UNKNOWN;
        consume();
        return op;
    }

    if (t.type == TokenType::BOOL_LIT) {
        op.kind     = Operand::Kind::CONST_BOOL;
        op.type     = IECType::BOOL;
        op.constVal = (t.value == "TRUE");
        consume();
    } else if (t.type == TokenType::REAL_LIT) {
        op.kind     = Operand::Kind::CONST_REAL;
        op.type     = IECType::REAL;
        op.constVal = std::stof(t.value);
        consume();
    } else if (t.type == TokenType::INT_LIT) {
        int32_t v   = std::stol(t.value);
        op.type     = IECType::DINT;
        op.kind     = Operand::Kind::CONST_DINT;
        op.constVal = v;
        consume();
    } else if (t.type == TokenType::IDENTIFIER) {
        op.kind   = Operand::Kind::VARIABLE;
        op.strVal = t.value;
        op.type   = IECType::UNKNOWN;
        consume();
    } else if (t.type == TokenType::LABEL) {
        // JMP target
        op.kind   = Operand::Kind::LABEL_REF;
        op.strVal = t.value;
        consume();
    } else {
        error("Unexpected operand token: '" + t.value + "'");
    }
    return op;
}

IECType Parser::parseType() {
    switch (consume().type) {
        case TokenType::TYPE_BOOL: return IECType::BOOL;
        case TokenType::TYPE_INT:  return IECType::INT;
        case TokenType::TYPE_DINT: return IECType::DINT;
        case TokenType::TYPE_REAL: return IECType::REAL;
        case TokenType::TYPE_TIME: return IECType::TIME;
        default: return IECType::UNKNOWN;
    }
}
