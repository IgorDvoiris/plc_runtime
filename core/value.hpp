// value.hpp
#pragma once
#include <cstdint>
#include <string>
#include <stdexcept>
#include <cstring>

enum class ValueType : uint8_t {
    BOOL  = 0,
    INT   = 1,
    DINT  = 2,
    REAL  = 3,
    TIME  = 4,
    NONE  = 0xFF
};

struct Value {
    ValueType type = ValueType::NONE;
    union {
        bool    b;
        int16_t i;
        int32_t di;
        float   r;
        int32_t t;
    } data{};

    static Value fromBool (bool    v);
    static Value fromInt  (int16_t v);
    static Value fromDInt (int32_t v);
    static Value fromReal (float   v);
    static Value fromTime (int32_t v);

    bool    asBool() const;
    int32_t asInt()  const;
    float   asReal() const;

    Value operator+(const Value& o) const;
    Value operator-(const Value& o) const;
    Value operator*(const Value& o) const;
    Value operator/(const Value& o) const;
    Value operator%(const Value& o) const;

    bool operator>(const Value& o)  const;
    bool operator>=(const Value& o) const;
    bool operator<(const Value& o)  const;
    bool operator<=(const Value& o) const;
    bool operator==(const Value& o) const;
    bool operator!=(const Value& o) const;

    Value logicAnd(const Value& o) const;
    Value logicOr (const Value& o) const;
    Value logicXor(const Value& o) const;
    Value logicNot()               const;

private:
    Value applyArith(const Value& o, char op) const;
    int   cmp(const Value& o) const;
};
