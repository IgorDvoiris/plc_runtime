// value.cpp
#include <core/io/value.hpp>

Value Value::fromBool (bool    v) { Value x; x.type=ValueType::BOOL;  x.data.b  = v; return x; }
Value Value::fromInt  (int16_t v) { Value x; x.type=ValueType::INT;   x.data.i  = v; return x; }
Value Value::fromDInt (int32_t v) { Value x; x.type=ValueType::DINT;  x.data.di = v; return x; }
Value Value::fromReal (float   v) { Value x; x.type=ValueType::REAL;  x.data.r  = v; return x; }
Value Value::fromTime (int32_t v) { Value x; x.type=ValueType::TIME;  x.data.t  = v; return x; }

bool Value::asBool() const {
    switch (type) {
        case ValueType::BOOL: return data.b;
        case ValueType::INT:  return data.i  != 0;
        case ValueType::DINT: return data.di != 0;
        default:              return false;
    }
}

int32_t Value::asInt() const {
    switch (type) {
        case ValueType::BOOL: return data.b ? 1 : 0;
        case ValueType::INT:  return data.i;
        case ValueType::DINT: return data.di;
        case ValueType::REAL: return static_cast<int32_t>(data.r);
        default:              return 0;
    }
}

float Value::asReal() const {
    switch (type) {
        case ValueType::BOOL: return data.b ? 1.0f : 0.0f;
        case ValueType::INT:  return static_cast<float>(data.i);
        case ValueType::DINT: return static_cast<float>(data.di);
        case ValueType::REAL: return data.r;
        default:              return 0.0f;
    }
}

Value Value::applyArith(const Value& o, char op) const {
    if (type == ValueType::REAL || o.type == ValueType::REAL) {
        float a = asReal(), b = o.asReal();
        switch (op) {
            case '+': return fromReal(a + b);
            case '-': return fromReal(a - b);
            case '*': return fromReal(a * b);
            case '/':
                if (b == 0.0f) throw std::runtime_error("Division by zero");
                return fromReal(a / b);
        }
    }
    int32_t a = asInt(), b = o.asInt();
    switch (op) {
        case '+': return fromDInt(a + b);
        case '-': return fromDInt(a - b);
        case '*': return fromDInt(a * b);
        case '/':
            if (b == 0) throw std::runtime_error("Division by zero");
            return fromDInt(a / b);
    }
    return fromDInt(0);
}

int Value::cmp(const Value& o) const {
    if (type == ValueType::REAL || o.type == ValueType::REAL) {
        float d = asReal() - o.asReal();
        return (d > 0.0f) - (d < 0.0f);
    }
    int32_t a = asInt(), b = o.asInt();
    return (a > b) - (a < b);
}

Value Value::operator+(const Value& o) const { return applyArith(o, '+'); }
Value Value::operator-(const Value& o) const { return applyArith(o, '-'); }
Value Value::operator*(const Value& o) const { return applyArith(o, '*'); }
Value Value::operator/(const Value& o) const { return applyArith(o, '/'); }

Value Value::operator%(const Value& o) const {
    int32_t a = asInt(), b = o.asInt();
    if (b == 0) throw std::runtime_error("Division by zero (MOD)");
    return fromDInt(a % b);
}

bool Value::operator>(const Value& o)  const { return cmp(o) >  0; }
bool Value::operator>=(const Value& o) const { return cmp(o) >= 0; }
bool Value::operator<(const Value& o)  const { return cmp(o) <  0; }
bool Value::operator<=(const Value& o) const { return cmp(o) <= 0; }
bool Value::operator==(const Value& o) const { return cmp(o) == 0; }
bool Value::operator!=(const Value& o) const { return cmp(o) != 0; }

Value Value::logicAnd(const Value& o) const { return fromBool(asBool() && o.asBool()); }
Value Value::logicOr (const Value& o) const { return fromBool(asBool() || o.asBool()); }
Value Value::logicXor(const Value& o) const { return fromBool(asBool() ^  o.asBool()); }
Value Value::logicNot()               const { return fromBool(!asBool()); }
